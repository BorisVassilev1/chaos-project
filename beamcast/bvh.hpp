#pragma once

/// @file bvh.h
/// @brief Primitive intersection acceleration
///
/// COPY PASTA FROM https://github.com/BorisVassilev1/urban-spork
/// with a lot of edits, of course

#include <cstddef>
#include <cstring>
#include <vector>
#include <memory>
#include <float.h>

#include <materials.hpp>
#include <myglm/myglm.h>
#include <data.hpp>
#include <intersectable.hpp>
#include <log.hpp>

namespace ygl {
/**
 * @brief Everything related to ray tracing acceleration structures
 *
 */
namespace bvh {

/// Interface for an acceleration structure for any intersectable primitives
template <class Element>
struct IntersectionAccelerator {
	enum class Purpose { Generic, Mesh, Instances };

	/// @brief Add the primitive to the accelerated list
	/// @param prim - non owning pointer
	virtual void addPrimitive(const Element el) = 0;

	/// @brief Clear all data allocated by the accelerator
	virtual void clear() = 0;

	/// @brief Build all the internal data for the accelerator
	///	@param purpose - the purpose of the tree, implementation can use it as hint for internal parameters
	virtual void build(Purpose purpose = Purpose::Generic) = 0;

	/// @brief Check if the accelerator is built
	virtual bool isBuilt() const = 0;

	using Filter = std::function<bool(const Element &)>;
	virtual bool intersect(const Ray &ray, float tMin, float tMax, RayHit &intersection, const Filter &f) const = 0;

	virtual ~IntersectionAccelerator() = default;
};

template <class Element>
using AcceleratorPtr = std::unique_ptr<IntersectionAccelerator<Element>>;

template <class Element>
class FakePointer {
	Element element;

   public:
	inline constexpr FakePointer(Element el) : element(el) {}
	inline constexpr FakePointer(std::nullptr_t) : element(nullptr) {}

	inline constexpr const Element *operator->() const { return &element; }
	inline constexpr Element	   *operator->() { return &element; }
	inline constexpr Element	   &operator*() { return element; }
	inline constexpr const Element &operator*() const { return element; }
	inline constexpr				operator bool() const { return (bool)element; }
	inline constexpr bool			operator!() const { return !element; }

	inline constexpr Element	   &get() { return element; }
	inline constexpr const Element &get() const { return element; }
};

/**
 * @brief A Binary Volume Hierarchy.
 * build() builds a tree that can then be optimised for GPU usage and sent to the VRAM with buildGPUTree()
 *
 * @note some comments may be misleading since now this structure is used to do stuff on the GPU, the CPU intersection
 * has been removed
 */
template <class Element>
class BVHTree : public IntersectionAccelerator<Element> {
	using ElementOwn = std::conditional_t<std::is_pointer_v<Element>,
										  std::unique_ptr<std::remove_pointer_t<const Element>>, FakePointer<Element>>;
	using ElementPtr = std::conditional_t<std::is_pointer_v<Element>, Element, Element *>;
	using Super		 = IntersectionAccelerator<Element>;

	// Node structure
	// does not need a parent pointer since the intersection implementation will be recursive
	// I can afford that since the code will run on CPU which will not suffer from it.
	// This will simplify the implementation a lot.
	struct Node {
		AABB					box;
		std::unique_ptr<Node>	children[2];
		std::vector<ElementOwn> primitives;
		char					splitAxis;
		Node() : children{nullptr, nullptr}, splitAxis(-1) {}
		bool		isLeaf() { return children[0] == nullptr; }
		auto	   &left() { return children[0]; }			  ///< returns the left child
		const auto &left() const { return children[0]; }	  ///< returns the left child
		auto	   &right() { return children[1]; }			  ///< returns the right child
		const auto &right() const { return children[1]; }	  ///< returns the right child
	};

	// faster intersection tree node;
	// left child will always be next in the array, right child is a index in the nodes array.
	struct FastNode {
		AABB		box;
		std::size_t right;	   // 4e9 is a puny number of primitives => long, not int
		std::size_t primitives;

		char splitAxis;
		bool isLeaf() const {
			// 0th node will always be the root so no node points to it as its right child
			return right == 0;
		}
	};

	struct alignas(16) GPUNode {
		vec3 min;
		uint parent;
		vec3 max;
		uint right;
		uint primOffset;
		uint primCount;
		bool isLeaf() { return right == 0; }
	};

	// all primitives added
	std::vector<ElementOwn> allPrimitives;
	// root of the construction tree
	std::unique_ptr<Node> root;

	// nodes of the fast traversal tree
	std::vector<FastNode> fastNodes;
	// the primitives sorted for fast traversal

	bool built = false;
	// cost for traversing a parent node. It is assumed that the intersection cost with a primitive is 1.0
	static constexpr float SAH_TRAVERSAL_COST = 0.125;
	// the number of splits SAH will try.
	static constexpr int SAH_TRY_COUNT		  = 5;
	static constexpr int MAX_DEPTH			  = 50;
	static constexpr int MIN_PRIMITIVES_COUNT = 6;
	// when a node has less than that number of primitives it will sort them and always split in the middle
	//
	// theoretically:
	// the higher this is, the better the tree and the slower its construction
	//
	// practically:
	// when setting this to something higher, the tree construction time changes by verry little
	// and the render time goes up. When this is set to 1e6 (basically always sort), makes
	// rendering about 30% slower than when it is 0 (never split perfectly). (on the instanced dragons scene)
	// Perfect splits clearly produce shallower trees, which should make rendering faster ... but it doesn't.
	//
	// I guess SAH is too good
	static constexpr int PERFECT_SPLIT_THRESHOLD = 20;

	int		 depth			 = 0;	  ///< depth of the tree
	int		 leafSize		 = 0;	  ///< size of the largest leaf
	long int leavesCount	 = 0;	  ///< hOw MaNy LeAvEs
	long int nodeCount		 = 0;	  ///< HoW mAnY nOdEs
	long int primitivesCount = 0;	  ///< how many primitives are in the structure

	void clearConstructionTree();							///< clears the entire CPU tree
	void build(std::unique_ptr<Node> &node, int depth);		///< builds the CPU tree

	/// @brief computes the SAH cost for a split on a given axis.
	/// ratio equals the size of the left child on the chosen axis over
	/// the size of the parent node size on that axis.
	float costSAH(const std::unique_ptr<Node> &node, int axis, float ratio);

   public:
	using typename Super::Purpose;

	BVHTree()							= default;
	BVHTree(const BVHTree &)			= delete;
	BVHTree(BVHTree &&)					= default;
	BVHTree &operator=(const BVHTree &) = delete;
	BVHTree &operator=(BVHTree &&)		= default;

	bool isBuilt() const override { return built; }		///< checks if the tree is built

	void addPrimitive(const Element prim) override;
	/**
	 * @brief Adds all triangles in the given \a mesh and translates them with \a transform.
	 *
	 * @param mesh - the mesh to be added to the BVH
	 * @param transform - a Transformation for the model
	 */
	// void addPrimitive(Mesh *mesh, Transformation &transform);
	void clear() override;
	void build(Super::Purpose purpose = Super::Purpose::Generic) override;

	void	 buildFastTree();
	void	 buildFastTree(std::unique_ptr<Node> &, std::vector<FastNode> &allNodes);
	FastNode makeFastLeaf(std::unique_ptr<Node> &node);

	bool intersect(
		long unsigned int nodeIndex, const Ray &ray, float tMin, float &tMax, RayHit &intersection,
		const Super::Filter &f = [](const Element &) { return true; }) const;
	bool intersect(
		const Ray &ray, float tMin, float tMax, RayHit &intersection,
		const Super::Filter &f = [](const Element &) { return true; }) const override;

	~BVHTree();

	const auto &getObjects() const {return allPrimitives;}
};

using TriangleBVH = BVHTree<Triangle>;
using AbstractBVH = BVHTree<Intersectable *>;

template <class Element>
void BVHTree<Element>::addPrimitive(const Element prim) {
	allPrimitives.emplace_back(prim);
}

template <class Element>
void BVHTree<Element>::clear() {
	clearConstructionTree();
	allPrimitives.clear();
	fastNodes.clear();
	built = false;
}

template <class Element>
void BVHTree<Element>::clearConstructionTree() {
	root.reset(nullptr);
}

template <class Element>
void BVHTree<Element>::build(std::unique_ptr<Node> &node, int depth) {
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
		node->left()  = std::make_unique<Node>();
		node->right() = std::make_unique<Node>();
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
		node->left()  = std::make_unique<Node>();
		node->right() = std::make_unique<Node>();
		nodeCount += 2;
		for (auto &obj : node->primitives) {
			int ind = obj->getCenter()[maxAxis] > split;
			obj->expandBox(node->children[ind]->box);
			node->children[ind]->primitives.push_back(std::move(obj));
		}
	}

	build(node->left(), depth + 1);
	build(node->right(), depth + 1);
	node->primitives.clear();
}

template <class Element>
void BVHTree<Element>::build(Super::Purpose purpose) {
	static_cast<void>(purpose);
	// purpose is ignored. what works best for triangles seems to also work best for objects
	printf("Building BVH tree with %d primitives... \n", int(allPrimitives.size()));
	fflush(stdout);
	Timer timer;

	primitivesCount = allPrimitives.size();

	root = std::make_unique<Node>();
	root->primitives.swap(allPrimitives);
	for (unsigned long int c = 0; c < root->primitives.size(); ++c) {
		root->primitives[c]->expandBox(root->box);
	}
	// build both trees
	Timer buildTimer;
	build(root, 0);
	dbLog(dbg::LOG_INFO, "Main Tree built: ", buildTimer.elapsed<std::chrono::milliseconds>(), "ms");

	buildFastTree();

	// construction tree is no longer needed
	clearConstructionTree();

	built = true;
	dbLog(dbg::LOG_INFO, " done in ", timer.elapsed<std::chrono::milliseconds>(), " ms, nodes: ", nodeCount,
		  ", leaves: ", leavesCount, ", depth: ", depth, ", leaf size: ", leafSize);
}

template <class Element>
float BVHTree<Element>::costSAH(const std::unique_ptr<Node> &node, int axis, float ratio) {
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

template <class Element>
BVHTree<Element>::~BVHTree() {
	clear();
	clearConstructionTree();
}

template <class Element>
bool BVHTree<Element>::intersect(long unsigned int nodeIndex, const Ray &ray, float tMin, float &tMax,
								 RayHit &intersection, const IntersectionAccelerator<Element>::Filter &f) const {
	bool			hasHit = false;
	const FastNode *node   = &(fastNodes[nodeIndex]);

	if (node->isLeaf()) {
		// dbLog(dbg::LOG_DEBUG, "Intersecting leaf ", nodeIndex, " with ", node->primitives, " primitives");
		//   iterate either to a invalid pointer or to the max leaf size
		assert(node->primitives != -1ull && "Primitive pointer is null in BVH tree");
		for (int i = 0; i < leafSize && allPrimitives[node->primitives + i]; i++) {
			const auto &prim = allPrimitives[node->primitives + i];
			assert(prim && "Primitive pointer is null in BVH tree");
			if (f(prim.get()) && prim->intersect(ray, tMin, tMax, intersection)) {
				tMax					 = intersection.t;
				hasHit					 = true;
				intersection.objectIndex = node->primitives + i;
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

template <class Element>
bool BVHTree<Element>::intersect(const Ray &ray, float tMin, float tMax, RayHit &intersection,
								 const IntersectionAccelerator<Element>::Filter &f) const {
	if (!allPrimitives.empty() && fastNodes[0].box.testIntersect(ray)) {
		return intersect(0, ray, tMin, tMax, intersection, f);
	} else return false;
}

// builds a tree for fast traversal
template <class Element>
void BVHTree<Element>::buildFastTree() {
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

template <class Element>
void BVHTree<Element>::buildFastTree(std::unique_ptr<Node> &node, std::vector<FastNode> &allNodes) {
	int parentIndex = allNodes.size() - 1;

	// insert the left child
	allNodes.push_back(makeFastLeaf(node->left()));

	// trace down left
	if (!(node->left()->isLeaf())) { buildFastTree(node->left(), allNodes); }

	// add right child
	allNodes.push_back(makeFastLeaf(node->right()));

	// write to the parent right pointer
	allNodes[parentIndex].right = allNodes.size() - 1;

	// trace down right
	if (!(node->right()->isLeaf())) { buildFastTree(node->right(), allNodes); }
}

// copies the data from a Node to a FastNode when constructing the traversal tree
template <class Element>
BVHTree<Element>::FastNode BVHTree<Element>::makeFastLeaf(std::unique_ptr<Node> &node) {
	if (node->isLeaf()) {
		// This should have sped up things because continuous allocation should be better for cache.
		// But it has no effect. It looks like the access of primitives is random enough that
		// it always generates a cache miss.
		const auto begin_index = allPrimitives.size();
		for (auto &prim : node->primitives) {
			allPrimitives.emplace_back(std::move(prim));
		}
		allPrimitives.emplace_back(nullptr);
		assert(!allPrimitives.back() && "Last primitive in the leaf must be null");
		node->primitives.clear();

		return FastNode{node->box, 0, begin_index, node->splitAxis};
	} else {
		return FastNode{node->box, 0, -1ull, node->splitAxis};
	}
}

}	  // namespace bvh
}	  // namespace ygl
