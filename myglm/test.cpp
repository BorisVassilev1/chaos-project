#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

#include <myglm/myglm.h>

TEST_CASE("Sanity Check") { CHECK(1 + 1 == 2); }

TEST_CASE("apply") {
	vec<int, 3> v1{1, 2, 3};
	vec<int, 3> v2{4, 5, 6};

	vec<int, 3> result = apply2(v1, v2, [](int a, int b) { return a + b; });

	CHECK(result[0] == 5);
	CHECK(result[1] == 7);
	CHECK(result[2] == 9);
	CHECK(result.size() == 3);
}

TEST_CASE("operators") {
	vec<int, 3> v1{1, 2, 3};
	vec<int, 3> v2{4, 5, 6};

	auto sum = v1 + v2;
	CHECK(sum[0] == 5);
	CHECK(sum[1] == 7);
	CHECK(sum[2] == 9);

	auto diff = v1 - v2;
	CHECK(diff[0] == -3);
	CHECK(diff[1] == -3);
	CHECK(diff[2] == -3);

	auto p = diff / -3;
	CHECK(p == vec<int, 3>(1));
}

TEST_CASE("sizes") {
	vec<int, 2> v0{0, 1};
	vec<int, 3> v1{1, 2, 3};
	vec<int, 4> v2{4, 5, 6, 7};

	CHECK(sizeof(v0) == sizeof(int) * 2);
	CHECK(sizeof(v1) == sizeof(int) * 3);
	CHECK(sizeof(v2) == sizeof(int) * 4);

	CHECK(v1.size() == 3);
	CHECK(v2.size() == 4);

	CHECK(v1.x == 1);
	CHECK(v1.y == 2);
	CHECK(v1.z == 3);
	CHECK(v2.x == 4);
	CHECK(v2.y == 5);
	CHECK(v2.z == 6);
	CHECK(v2.w == 7);

	CHECK(&v2.x == &v2.r);
	CHECK(&v2.y == &v2.g);
	CHECK(&v2.z == &v2.b);
	CHECK(&v2.w == &v2.a);
}

TEST_CASE("matrix") {
	mat<float, 2, 2> m2{vec<float,2>{1.f, 2.f}, vec<float,2>{3.f, 4.f}}; 
	mat<float, 2, 2> m3{{1.f, 2.f}, {3.f, 4.f}};
}

TEST_CASE("matrix multiplication") {
	mat<float, 2, 2> m1{{1.f, 1.f}, {1.f, -1.f}};
	mat<float, 2, 2> m2{{1.f, 1.f}, {1.f, -1.f}};

	auto result = m1 * m2;

	CHECK(result[0][0] == 2.f);
	CHECK(result[0][1] == 0.f);
	CHECK(result[1][0] == 0.f);
	CHECK(result[1][1] == 2.f);

	CHECK(result == mat2{{2.f, 0.f}, {0.f, 2.f}});

	CHECK(m1 + m2 == mat2{{2.f, 2.f}, {2.f, -2.f}});
}

TEST_CASE("matrix transpose") {
	mat<float, 2, 3> m1{{1.f, 2.f, 3.f}, {4.f, 5.f, 6.f}};

	auto t = Transpose(m1);

	t[0] = vec<float, 2>{7.f, 8.f};
	
	CHECK(m1 == mat<float, 2, 3>{{7.f, 2.f, 3.f}, {8.f, 5.f, 6.f}});
};

TEST_CASE("composition") {
	vec2 v1{1.f, 2.f};
	vec2 v2{3.f, 4.f};
	vec4 v3(v1, 1.f, 2.f);
}
