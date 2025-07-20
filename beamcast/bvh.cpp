#include <bvh.hpp>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <memory>
#include <vector>

#include <log.hpp>
#include "intersectable.hpp"

using namespace ygl::bvh;

AcceleratorPtr makeDefaultAccelerator() { return AcceleratorPtr(new BVHTree()); }

void BVHTree::addPrimitive(const Intersectable *prim) { allPrimitives.emplace_back(prim); }

void BVHTree::clear(Node *node) {
	if (node == nullptr) return;
	for (int i = 0; i < 2; i++) {
		clear(node->children[i]);
		delete node->children[i];
	}
}

void BVHTree::clear() {
	if (root != nullptr) clear(root);
	clearConstructionTree();
	// allPrimitives.clear();
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
	AABB centerBox;
	for (const auto &obj : node->primitives) {
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
			[&](const auto &a, const auto &b) { return a->getCenter()[maxAxis] < b->getCenter()[maxAxis]; });
		node->left	= new Node();
		node->right = new Node();
		nodeCount += 2;

		// split in half
		for (unsigned long int i = 0; i < size; ++i) {
			bool ind = i > (size / 2 - 1);
			node->primitives[i]->expandBox(node->children[ind]->box);
			node->children[ind]->primitives.emplace_back(std::move(node->primitives[i]));
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
		for (auto &obj : node->primitives) {
			int ind = obj->getCenter()[maxAxis] > split;
			obj->expandBox(node->children[ind]->box);
			node->children[ind]->primitives.push_back(std::move(obj));
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
	dbLog(dbg::LOG_INFO, "Main Tree built: ", buildTimer.elapsed<std::chrono::milliseconds>());

	buildFastTree();

	// construction tree is no longer needed
	clearConstructionTree();

	built = true;
	dbLog(dbg::LOG_INFO, " done in ", timer.elapsed<std::chrono::milliseconds>(), " ms, nodes: ", nodeCount,
		  ", leaves: ", leavesCount, ", depth: ", depth, ", leaf size: ", leafSize);
}

float BVHTree::costSAH(const Node *node, int axis, float ratio) {
	// effectively a lerp between the min and max of the box
	const float split	 = node->box.min[axis] * ratio + node->box.max[axis] * (1 - ratio);
	long int	count[2] = {0, 0};
	AABB		boxes[2];
	// count indices and merge bounding boxes in both children
	for (const auto &obj : node->primitives) {
		int ind = (obj->getCenter()[axis] > split);
		++count[ind];
		obj->expandBox(boxes[ind]);
	}
	// just the formula
	float s0   = node->box.surfaceArea();
	float s[2] = {count[0] ? boxes[0].surfaceArea() : 0, count[1] ? boxes[1].surfaceArea() : 0};
	return SAH_TRAVERSAL_COST + (s[0] * count[0] + s[1] * count[1]) / s0;
}

BVHTree::~BVHTree() {
	clear();
	clearConstructionTree();
}

bool BVHTree::intersect(long unsigned int nodeIndex, const Ray &ray, float tMin, float &tMax,
						RayHit &intersection) const {
	bool			hasHit = false;
	const FastNode *node   = &(fastNodes[nodeIndex]);

	if (node->isLeaf()) {
		// dbLog(dbg::LOG_DEBUG, "Intersecting leaf ", nodeIndex, " with ", node->primitives, " primitives");
		//   iterate either to a invalid pointer or to the max leaf size
		assert(node->primitives != nullptr && "Primitive pointer is null in BVH tree");
		for (int i = 0; i < leafSize && node->primitives[i] != nullptr; i++) {
			assert(node->primitives[i] != nullptr && "Primitive pointer is null in BVH tree");
			if (node->primitives[i]->intersect(ray, tMin, tMax, intersection)) {
				tMax   = intersection.t;
				hasHit = true;
			}
		}
	} else {
		// If the ray moves towards the positive direction, then will traverse in order left->right
		// and right->left if else.
		// First intersect with both child bounding boxes. Intersection with things inside the second box can
		// be skipped if distance to intersection found in the first one is closer than that with the second box
		//				 left 		 right
		//             +---------+----+--------+
		//       ray   |         |    |        |
		//     --------+-->/\    |    |   /\   |
		//             |  /--\   |    |  /--\  |
		//             |         |    |        |
		//             +---------+----+        |
		//                       |             |
		//                       +-------------+

		// I know that the next section is almost unreadable,
		// won't be much better if I replace it with a lot of copy-pasted if-statements

		// the distances to both children boxes
		float			  dist[2];
		long unsigned int childIndices[2]  = {nodeIndex + 1, node->right};
		bool			  testIntersect[2] = {fastNodes[childIndices[0]].box.testIntersect(ray, dist[0]),
											  fastNodes[childIndices[1]].box.testIntersect(ray, dist[1])};

		int direction = ray.direction[node->splitAxis] > 0.f;

		if (testIntersect[!direction]) {
			if (intersect(childIndices[!direction], ray, tMin, tMax, intersection)) {
				tMax   = intersection.t;
				hasHit = true;
			}
		}
		// if the closest found intersection is farther than the intersection
		// with the second box, traverse it, otherwise skip it
		if (tMax > dist[direction] && testIntersect[direction]) {
			if (intersect(childIndices[direction], ray, tMin, tMax, intersection)) {
				tMax   = intersection.t;
				hasHit = true;
			}
		}
	}

	return hasHit;
}

bool BVHTree::intersect(const Ray &ray, float tMin, float tMax, RayHit &intersection) const {
	if (fastNodes[0].box.testIntersect(ray)) {
		return intersect(0, ray, tMin, tMax, intersection);
	} else return false;
}

// builds a tree for fast traversal
void BVHTree::buildFastTree() {
	// no reallocations whould happen
	fastNodes.reserve(nodeCount);

	// every leaf will have a list of primitives that ends with a null pointer.
	assert(allPrimitives.empty() && "All primitives should be empty before building the fast tree");
	allPrimitives.reserve(primitivesCount + leavesCount);

	// the other function expects the parent node to already have been pushed to the vector
	fastNodes.push_back(makeFastLeaf(root));
	// leaves must not be traced down
	if (!(root->isLeaf())) { buildFastTree(root, fastNodes); }
}

void BVHTree::buildFastTree(Node *node, std::vector<FastNode> &allNodes) {
	int parentIndex = allNodes.size() - 1;

	// insert the left child
	allNodes.push_back(makeFastLeaf(node->left));

	// trace down left
	if (!(node->left->isLeaf())) { buildFastTree(node->left, allNodes); }

	// add right child
	allNodes.push_back(makeFastLeaf(node->right));

	// write to the parent right pointer
	allNodes[parentIndex].right = allNodes.size() - 1;

	// trace down right
	if (!(node->right->isLeaf())) { buildFastTree(node->right, allNodes); }
}

// copies the data from a Node to a FastNode when constructing the traversal tree
BVHTree::FastNode BVHTree::makeFastLeaf(Node *node) {
	if (node->isLeaf()) {
		// This should have sped up things because continuous allocation should be better for cache.
		// But it has no effect. It looks like the access of primitives is random enough that
		// it always generates a cache miss.
		const auto begin_index = allPrimitives.size();
		for (auto &prim : node->primitives) {
			allPrimitives.emplace_back(std::move(prim));
		}
		allPrimitives.emplace_back(nullptr);
		node->primitives.clear();

		const auto &begin = allPrimitives.data() + begin_index;
		static_assert(sizeof(std::unique_ptr<const Intersectable>) == sizeof(const Intersectable *),
					  "The size of unique_ptr must be the same as a pointer to Intersectable");
		const Intersectable **primitives = reinterpret_cast<const Intersectable **>(begin);

		return FastNode{node->box, 0, primitives, node->splitAxis};
	} else {
		return FastNode{node->box, 0, nullptr, node->splitAxis};
	}
}
