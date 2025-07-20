#include <bvh.hpp>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <vector>

using namespace ygl::bvh;

BBox::BBox(const vec3 &min, const vec3 &max) : min(min), max(max) {}

bool BBox::isEmpty() const {
	const vec3 size = max - min;
	return size.x <= 1e-6f || size.y <= 1e-6f || size.z <= 1e-6f;
}

void BBox::add(const BBox &other) {
	min = ::min(min, other.min);
	max = ::max(max, other.max);
}

void BBox::add(const vec3 &point) {
	min = ::min(min, point);
	max = ::max(max, point);
}

bool BBox::inside(const vec3 &point) const {
	return (min.x - 1e-6 <= point.x && point.x <= max.x + 1e-6 && min.y - 1e-6 <= point.y && point.y <= max.y + 1e-6 &&
			min.z - 1e-6 <= point.z && point.z <= max.z + 1e-6);
}

void BBox::octSplit(BBox parts[8]) const {
	assert(!isEmpty());
	const vec3 size   = max - min;
	const vec3 center = min + size / 2.f;

	parts[0] = BBox(min, center);
	parts[1] = BBox(center, max);

	parts[2] = BBox(vec3{min.x, center.y, min.z}, vec3{center.x, max.y, center.z});
	parts[3] = BBox(vec3{min.x, center.y, center.z}, vec3{center.x, max.y, max.z});
	parts[4] = BBox(vec3{min.x, min.y, center.z}, vec3{center.x, center.y, max.z});

	parts[6] = BBox(vec3{center.x, min.y, center.z}, vec3{max.x, center.y, max.z});
	parts[5] = BBox(vec3{center.x, min.y, min.z}, vec3{max.x, center.y, center.z});
	parts[7] = BBox(vec3{center.x, center.y, min.z}, vec3{max.x, max.y, center.z});
}

BBox BBox::boxIntersection(const BBox &other) const {
	assert(!isEmpty());
	assert(!other.isEmpty());
	return {::max(min, other.min), ::min(max, other.max)};
}

bool BBox::testIntersect(const Ray &ray, float &t) const {
	// Source:
	// https://medium.com/@bromanz/another-view-on-the-classic-ray-aabb-intersection-algorithm-for-bvh-traversal-41125138b525
	float t1 = -FLT_MAX;
	float t2 = FLT_MAX;

	vec3 t0s = (min - ray.origin) * (vec3(1.0f) / ray.direction);
	vec3 t1s = (max - ray.origin) * (vec3(1.0f) / ray.direction);

	vec3 tsmaller = ::min(t0s, t1s);
	vec3 tbigger  = ::max(t0s, t1s);

	t1 = std::max(t1, std::max(tsmaller.x, std::max(tsmaller.y, tsmaller.z)));
	t2 = std::min(t2, std::min(tbigger.x, std::min(tbigger.y, tbigger.z)));
	t  = t1;
	return t1 <= t2;
}

bool BBox::testIntersect(const Ray &ray) const {
	float a;
	return testIntersect(ray, a);
}

vec3 BBox::center() const { return (min + max) / 2.f; }

float BBox::surfaceArea() const {
	vec3 size = max - min;
	return (size.x * size.y + size.x * size.z + size.y * size.z);
}

/// source https://github.com/anrieff/quaddamage/blob/master/src/mesh.cpp
bool intersectTriangleFast(const Ray &ray, const vec3 &A, const vec3 &B, const vec3 &C, float &dist) {
	const vec3 AB = B - A;
	const vec3 AC = C - A;
	const vec3 D  = -ray.direction;
	//              0               A
	const vec3 H = ray.origin - A;

	/* 2. Solve the equation:
	 *
	 * A + lambda2 * AB + lambda3 * AC = ray.start + gamma * ray.dir
	 *
	 * which can be rearranged as:
	 * lambda2 * AB + lambda3 * AC + gamma * D = ray.start - A
	 *
	 * Which is a linear system of three rows and three unknowns, which we solve using Carmer's rule
	 */

	// Find the determinant of the left part of the equation:
	const vec3 ABcrossAC = cross(AB, AC);
	const float		Dcr		  = dot(ABcrossAC, D);

	// are the ray and triangle parallel?
	if (fabs(Dcr) < 1e-12) { return false; }

	const float lambda2 = dot(cross(H, AC), D) / Dcr;
	const float lambda3 = dot(cross(AB, H), D) / Dcr;
	const float gamma	= dot(ABcrossAC, H) / Dcr;

	// is intersection behind us, or too far?
	if (gamma < 0 || gamma > dist) { return false; }

	// is the intersection outside the triangle?
	if (lambda2 < 0 || lambda2 > 1 || lambda3 < 0 || lambda3 > 1 || lambda2 + lambda3 > 1) { return false; }

	dist = gamma;

	return true;
}

AcceleratorPtr makeDefaultAccelerator() { return AcceleratorPtr(new BVHTree()); }

void BVHTree::addPrimitive(Intersectable *prim) { allPrimitives.push_back(prim); }

//#if !defined(YGL_NO_COMPUTE_SHADERS)
//void BVHTree::addPrimitive(ygl::Mesh *mesh, ygl::Transformation &transform) {
//	mat4x4 &mat		   = transform.getWorldMatrix();
//	std::size_t	 verticesCount = mesh->getVerticesCount();
//	std::size_t	 indicesCount  = mesh->getIndicesCount();
//
//	float	 *vertices = new float[verticesCount * 3];
//	uint32_t *indices  = new uint32_t[indicesCount * 3];
//	glBindBuffer(GL_ARRAY_BUFFER, mesh->getVertices().bufferId);
//	glGetBufferSubData(GL_ARRAY_BUFFER, 0, verticesCount * sizeof(float) * 3, vertices);
//	glBindBuffer(GL_ARRAY_BUFFER, 0);
//
//	glBindBuffer(GL_ARRAY_BUFFER, mesh->getIBO());
//	glGetBufferSubData(GL_ARRAY_BUFFER, 0, indicesCount * sizeof(uint32_t), indices);
//	glBindBuffer(GL_ARRAY_BUFFER, 0);
//
//	for (std::size_t i = 0; i < indicesCount; i += 3) {
//		uint32_t i0 = indices[i + 0];
//		uint32_t i1 = indices[i + 1];
//		uint32_t i2 = indices[i + 2];
//
//		vec3 v0 = vec3(vertices[i0 * 3 + 0], vertices[i0 * 3 + 1], vertices[i0 * 3 + 2]);
//		vec3 v1 = vec3(vertices[i1 * 3 + 0], vertices[i1 * 3 + 1], vertices[i1 * 3 + 2]);
//		vec3 v2 = vec3(vertices[i2 * 3 + 0], vertices[i2 * 3 + 1], vertices[i2 * 3 + 2]);
//
//		v0 = mat * vec4(v0, 1.0f);
//		v1 = mat * vec4(v1, 1.0f);
//		v2 = mat * vec4(v2, 1.0f);
//		this->addPrimitive(new Triangle(i0, i1, i2, v0, v1, v2, 0));
//	}
//	delete[] vertices;
//	delete[] indices;
//}
//#endif

void BVHTree::clear(Node *node) {
	if (node == nullptr) return;
	for (Intersectable *i : node->primitives) {
		delete i;
	}
	for (int i = 0; i < 2; i++) {
		clear(node->children[i]);
		delete node->children[i];
	}
}

void BVHTree::clear() {
	if (root != nullptr) clear(root);
	clearConstructionTree();
	allPrimitives.clear();
	built = false;
}

void BVHTree::clearConstructionTree() {
	clear(root);
	delete (root);
	root = nullptr;
}

void BVHTree::build(Node *node, int depth) {
	if (depth > MAX_DEPTH || node->primitives.size() <= MIN_PRIMITIVES_COUNT) {
		leafSize = std::max((int)(node->primitives.size()), leafSize);
		++leavesCount;
		return;
	}
	this->depth = std::max(depth, this->depth);

	// get a bounding box of all centroids
	BBox centerBox;
	for (Intersectable *obj : node->primitives) {
		centerBox.add(obj->getCenter());
	}
	vec3 size = centerBox.max - centerBox.min;

	// find the axis on which the box is largest
	char  maxAxis	   = -1;
	float maxAxisValue = -FLT_MAX;
	for (int i = 0; i < 3; ++i) {
		if (maxAxisValue < size[i]) {
			maxAxis		 = i;
			maxAxisValue = size[i];
		}
	}
	node->splitAxis = maxAxis;

	// choose splitting algorithm
	if (node->primitives.size() < PERFECT_SPLIT_THRESHOLD) {
		unsigned long int size = node->primitives.size();
		// sorts so that the middle element is in its place, all others are in sorted order relative to it
		std::nth_element(
			node->primitives.begin(), node->primitives.begin() + size * 0.5, node->primitives.end(),
			[&](Intersectable *&a, Intersectable *&b) { return a->getCenter()[maxAxis] < b->getCenter()[maxAxis]; });
		node->left	= new Node();
		node->right = new Node();
		nodeCount += 2;

		// split in half
		for (unsigned long int i = 0; i < size; ++i) {
			bool ind = i > (size / 2 - 1);
			node->primitives[i]->expandBox(node->children[ind]->box);
			node->children[ind]->primitives.push_back(node->primitives[i]);
		}
	} else {
		long int noSplitSAH = node->primitives.size();

		// try evenly distributed splits with SAH
		float bestSAH = FLT_MAX, bestRatio = -1;
		for (int i = 0; i < SAH_TRY_COUNT; ++i) {
			float ratio = float(i + 1.) / float(SAH_TRY_COUNT + 1);
			float sah	= costSAH(node, maxAxis, ratio);
			if (bestSAH > sah) {
				bestSAH	  = sah;
				bestRatio = ratio;
			}
		}

		// create a leaf when the node can't be split effectively
		if (bestSAH > noSplitSAH) {
			leafSize = std::max((int)(node->primitives.size()), leafSize);
			++leavesCount;
			return;
		}

		// position of the split plane. lerp between min and max
		const float split = node->box.min[maxAxis] * bestRatio + node->box.max[maxAxis] * (1 - bestRatio);
		// distibute the primitives to the child nodes
		node->left	= new Node();
		node->right = new Node();
		nodeCount += 2;
		for (Intersectable *obj : node->primitives) {
			int ind = obj->getCenter()[maxAxis] > split;
			obj->expandBox(node->children[ind]->box);
			node->children[ind]->primitives.push_back(obj);
		}
	}

	build(node->left, depth + 1);
	build(node->right, depth + 1);
	node->primitives.clear();
}

void BVHTree::build(Purpose purpose) {
	static_cast<void>(purpose);
	// purpose is ignored. what works best for triangles seems to also work best for objects
	printf("Building BVH tree with %d primitives... \n", int(allPrimitives.size()));
	fflush(stdout);
	Timer timer;

	primitivesCount = allPrimitives.size();

	root = new Node();
	root->primitives.swap(allPrimitives);
	for (unsigned long int c = 0; c < root->primitives.size(); ++c) {
		root->primitives[c]->expandBox(root->box);
	}
	// build both trees
	Timer buildTimer;
	build(root, 0);
	printf("Main Tree built: %lldms\n", (long long int)buildTimer.toMs(buildTimer.elapsedNs()));

	Timer gpuTimer;
	buildGPUTree();
	printf("GPU Tree built: %lldms\n", (long long int)gpuTimer.toMs(gpuTimer.elapsedNs()));

	// construction tree is no longer needed
	clearConstructionTree();

	built = true;
	printf(" done in %lldms, nodes: %ld, leaves: %ld, depth %d, %d leaf size\n",
		   (long long int)timer.toMs(timer.elapsedNs()), nodeCount, leavesCount, depth, leafSize);
}

float BVHTree::costSAH(const Node *node, int axis, float ratio) {
	// effectively a lerp between the min and max of the box
	const float split	 = node->box.min[axis] * ratio + node->box.max[axis] * (1 - ratio);
	long int	count[2] = {0, 0};
	BBox		boxes[2];
	// count indices and merge bounding boxes in both children
	for (Intersectable *obj : node->primitives) {
		int ind = (obj->getCenter()[axis] > split);
		++count[ind];
		obj->expandBox(boxes[ind]);
	}
	// just the formula
	float s0   = node->box.surfaceArea();
	float s[2] = {count[0] ? boxes[0].surfaceArea() : 0, count[1] ? boxes[1].surfaceArea() : 0};
	return SAH_TRAVERSAL_COST + (s[0] * count[0] + s[1] * count[1]) / s0;
}

void BVHTree::buildGPUTree_h(BVHTree::Node *node, unsigned long int parent, std::vector<BVHTree::GPUNode> &gpuNodes,
							 std::vector<Intersectable *> &primitives) {
	BVHTree::GPUNode current;
	current.min	   = node->box.min;
	current.max	   = node->box.max;
	current.parent = parent;
	if (node->isLeaf()) {
		current.right	   = 0;
		current.primOffset = primitives.size();
		current.primCount  = node->primitives.size();
		for (Intersectable *p : node->primitives) {
			primitives.push_back(p);
		}
	} else {
		current.primOffset = 0;
		current.primCount  = 0;
	}
	gpuNodes.push_back(current);
	unsigned long int currIndex = gpuNodes.size() - 1;

	if (node->isLeaf()) return;

	buildGPUTree_h(node->left, currIndex, gpuNodes, primitives);
	gpuNodes[currIndex].right = gpuNodes.size();
	buildGPUTree_h(node->right, currIndex, gpuNodes, primitives);
}

//#if !defined( YGL_NO_COMPUTE_SHADERS)
//void BVHTree::buildGPUTree() {
//	gpuNodes.reserve(nodeCount);
//	std::vector<Intersectable *> orderedPrimitives;
//
//	if (!(root->isLeaf())) buildGPUTree_h(root, 0, gpuNodes, orderedPrimitives);
//
//	// TODO: convert all primitives to GPU format and figure out how to access them by indices
//	// primitive data:
//	//		uint type = {box, sphere, triangle}
//	//		<28 bytes other data>
//	// total 32 bytes
//	std::size_t buffSize = 8 * sizeof(uint) * orderedPrimitives.size();
//	char	   *buff	 = new char[buffSize];
//	for (uint i = 0; i < orderedPrimitives.size(); ++i) {
//		orderedPrimitives[i]->writeTo(buff, i * 8 * sizeof(uint));
//	}
//
//	// send buff to gpu
//	GLuint primitivesBuff;
//	glGenBuffers(1, &primitivesBuff);
//	glBindBuffer(GL_SHADER_STORAGE_BUFFER, primitivesBuff);
//	glBufferData(GL_SHADER_STORAGE_BUFFER, buffSize, buff, GL_STATIC_DRAW);
//	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
//
//	ygl::Shader::setSSBO(primitivesBuff, 7);
//
//	GLuint nodesBuff;
//	glGenBuffers(1, &nodesBuff);
//	glBindBuffer(GL_SHADER_STORAGE_BUFFER, nodesBuff);
//	glBufferData(GL_SHADER_STORAGE_BUFFER, gpuNodes.size() * sizeof(GPUNode), gpuNodes.data(), GL_STATIC_DRAW);
//	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
//
//	ygl::Shader::setSSBO(nodesBuff, 5);
//	delete[] buff;
//}
//#endif
BVHTree::~BVHTree() {
	clear();
	clearConstructionTree();
}

