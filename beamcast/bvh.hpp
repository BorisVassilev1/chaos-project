#pragma once

/// @file bvh.h
/// @brief Primitive intersection acceleration
///
/// COPY PASTA FROM https://github.com/BorisVassilev1/urban-spork
/// with a lot of edits, of course

#include <cstring>
#include <ostream>
#include <vector>
#include <memory>
#include <float.h>

#include <materials.hpp>
#include <myglm/myglm.h>
#include <data.hpp>
#include <intersectable.hpp>

namespace ygl {
/**
 * @brief Everything related to ray tracing acceleration structures
 *
 */
namespace bvh {

enum PrimitiveType : uint {
	TRIANGLE = 0,
	SPHERE	 = 1,
	BOX		 = 2,
};

/// Interface for an acceleration structure for any intersectable primitives
struct IntersectionAccelerator {
	enum class Purpose { Generic, Mesh, Instances };

	/// @brief Add the primitive to the accelerated list
	/// @param prim - non owning pointer
	virtual void addPrimitive(const Intersectable *prim) = 0;

	/// @brief Clear all data allocated by the accelerator
	virtual void clear() = 0;

	/// @brief Build all the internal data for the accelerator
	///	@param purpose - the purpose of the tree, implementation can use it as hint for internal parameters
	virtual void build(Purpose purpose = Purpose::Generic) = 0;

	/// @brief Check if the accelerator is built
	virtual bool isBuilt() const = 0;

	virtual bool intersect(const Ray &ray, float tMin, float tMax, RayHit &intersection) const = 0;

	virtual ~IntersectionAccelerator() = default;
};

typedef std::unique_ptr<IntersectionAccelerator> AcceleratorPtr;
AcceleratorPtr									 makeDefaultAccelerator();


/**
 * @brief A Binary Volume Hierarchy.
 * build() builds a tree that can then be optimised for GPU usage and sent to the VRAM with buildGPUTree()
 *
 * @note some comments may be misleading since now this structure is used to do stuff on the GPU, the CPU intersection
 * has been removed
 */
class BVHTree : public IntersectionAccelerator {
	// Node structure
	// does not need a parent pointer since the intersection implementation will be recursive
	// I can afford that since the code will run on CPU which will not suffer from it.
	// This will simplify the implementation a lot.
	struct Node {
		AABB box;
		union {
			Node *children[2];
			struct {
				Node *left, *right;
			};
		};
		std::vector<std::unique_ptr<const Intersectable>> primitives;
		char							   splitAxis;
		Node() : left(nullptr), right(nullptr), splitAxis(-1) {}
		bool isLeaf() { return children[0] == nullptr; }

	};

	// faster intersection tree node;
	// left child will always be next in the array, right child is a index in the nodes array.
	struct FastNode {
		AABB						box;
		std::size_t					right;	   // 4e9 is a puny number of primitives => long, not int
		const Intersectable *const *primitives;
		char						splitAxis;
		bool						isLeaf() const {
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
	std::vector<std::unique_ptr<const Intersectable>> allPrimitives;
	// root of the construction tree
	Node *root = nullptr;

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

	void clear(Node *node);				   ///< clears the entire CPU tree starting from \a node
	void clearConstructionTree();		   ///< clears the entire CPU tree
	void build(Node *node, int depth);	   ///< builds the CPU tree

	/// @brief computes the SAH cost for a split on a given axis.
	/// ratio equals the size of the left child on the chosen axis over
	/// the size of the parent node size on that axis.
	float costSAH(const Node *node, int axis, float ratio);

   public:
	BVHTree() = default;
	BVHTree(const BVHTree &) = delete;
	BVHTree(BVHTree &&) = default;
	BVHTree &operator=(const BVHTree &) = delete;
	BVHTree &operator=(BVHTree &&) = default;

	bool isBuilt() const override { return built; }		///< checks if the tree is built

	void addPrimitive(const Intersectable *prim) override;
	/**
	 * @brief Adds all triangles in the given \a mesh and translates them with \a transform.
	 *
	 * @param mesh - the mesh to be added to the BVH
	 * @param transform - a Transformation for the model
	 */
	// void addPrimitive(Mesh *mesh, Transformation &transform);
	void clear() override;
	void build(Purpose purpose = Purpose::Generic) override;

	void	 buildFastTree();
	void	 buildFastTree(Node *node, std::vector<FastNode> &allNodes);
	FastNode makeFastLeaf(Node *node);

	bool intersect(long unsigned int nodeIndex, const Ray &ray, float tMin, float &tMax, RayHit &intersection) const;
	bool intersect(const Ray &ray, float tMin, float tMax, RayHit &intersection) const override;

	~BVHTree();
};
}	  // namespace bvh
}	  // namespace ygl
