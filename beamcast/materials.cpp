
#include <functional>
#include <materials.hpp>
#include <scene.hpp>

const float EPS		  = 0.001f;
const int	MAX_DEPTH = 5;

vec4 DiffuseMaterial::shade(const RayHit &hit, const Ray &, const Scene &scene) const {
	if (hit.depth >= MAX_DEPTH) { return scene.backgroundColor; }
	vec3 color = 0;
	for (const auto &light : scene.lights) {
		vec3  lightDir	 = light.position - hit.pos;
		float distanceSq = lengthSquared(lightDir);

		auto shadowHit = scene.intersect(Ray(hit.pos + hit.normal * EPS, lightDir));
		if (shadowHit.t > 0.f && shadowHit.t * shadowHit.t < distanceSq - EPS) {
			//	continue;	  // shadow
		}

		float distance = std::sqrt(distanceSq);

		color += light.color * light.intensity * std::max(0.f, dot(hit.normal, lightDir)) /
				 (4.f * M_PIf * distanceSq * distance);
	}
	return vec4(color * this->albedo, 1.0f);
}

vec4 ReflectiveMaterial::shade(const RayHit &hit, const Ray &ray, const Scene &scene) const {
	if (hit.depth >= MAX_DEPTH) { return scene.backgroundColor; }
	vec3   color		= 0;
	vec3   reflectedDir = normalize(reflect(ray.direction, hit.normal));
	Ray	   reflectedRay(hit.pos + hit.normal * EPS, reflectedDir);
	RayHit reflectedHit = scene.intersect(reflectedRay);
	if (reflectedHit.objectIndex != -1u) {
		reflectedHit.depth	 = hit.depth + 1;
		const auto	mat_id	 = scene.objects[reflectedHit.objectIndex].getMaterialIndex();
		const auto &material = scene.materials[mat_id];
		scene.fillHitInfo(reflectedHit, reflectedRay, material->smooth);
		color = material->shade(reflectedHit, reflectedRay, scene).xyz();
	} else {
		color = scene.backgroundColor.xyz();
	}
	return vec4(color * this->albedo, 1.0f);
}

static inline float F0(float ior) {
	float f = (ior - 1.0f) / (ior + 1.0f);
	return f * f;
}

static inline float FresnelSchlick(const vec3 &I, const vec3 &N, float f0) {
	float cosTheta = dot(I, N);
	return f0 + (1.0f - f0) * std::pow(std::clamp(1.0f - cosTheta, 0.0f, 1.0f), 5.0f);
}

static inline float fresnelReflectAmount(float n1, float n2, vec3 normal, vec3 incident, float f0, float f90) {
	// Schlick aproximation
	float r0 = (n1 - n2) / (n1 + n2);
	r0 *= r0;
	float cosX = -dot(normal, incident);
	if (n1 > n2) {
		float n		= n1 / n2;
		float sinT2 = n * n * (1.0 - cosX * cosX);
		// Total internal reflection
		if (sinT2 > 1.0) return f90;
		cosX = sqrt(1.0 - sinT2);
	}
	float x	  = 1.0 - cosX;
	float ret = r0 + (1.0 - r0) * x * x * x * x * x;
	// ret = clamp(ret, 0.0, 1.0);

	// adjust reflect multiplier for object reflectivity
	return f0 * (1.f - ret) + f90 * ret;
}

vec4 RefractiveMaterial::shade(const RayHit &hit, const Ray &ray, const Scene &scene) const {
	if (hit.depth >= MAX_DEPTH) { return scene.backgroundColor; }
	vec3  color			  = 0;
	float dotNormal		  = dot(hit.normal, ray.direction);
	bool  isEntering	  = dotNormal < 0.0f;
	float eta			  = isEntering ? 1.0f / ior : ior;	   // Snell's law
	vec3  correctedNormal = isEntering ? hit.normal : -hit.normal;
	vec3  normalEPS		  = correctedNormal * EPS;

	Ray reflectedRay(hit.pos + normalEPS, normalize(reflect(ray.direction, correctedNormal)));
	Ray refractedRay(hit.pos - normalEPS, normalize(refract(ray.direction, correctedNormal, eta)));

	float fresnel = fresnelReflectAmount(isEntering ? 1.0f : ior, isEntering ? ior : 1.0f, hit.normal, ray.direction,
										 F0(ior), 1.0f);

	RayHit reflectedHit	   = scene.intersect(reflectedRay);
	vec3   reflectionColor = vec3(0);
	if (reflectedHit.objectIndex != -1u) {
		reflectedHit.depth = hit.depth + 1;
		const auto mat_id  = scene.objects[reflectedHit.objectIndex].getMaterialIndex();
		assert(mat_id < scene.materials.size());
		const auto &material = scene.materials[mat_id];
		scene.fillHitInfo(reflectedHit, reflectedRay, material->smooth);
		reflectionColor = material->shade(reflectedHit, reflectedRay, scene)._xyz();
	} else {
		reflectionColor = scene.backgroundColor.xyz();
	}

	vec3 refractionColor = vec3(0);
	if (refractedRay.direction != vec3(0)) {
		RayHit refractedHit = scene.intersect(refractedRay);
		if (refractedHit.objectIndex != -1u) {
			refractedHit.depth = hit.depth + 1;
			const auto mat_id  = scene.objects[refractedHit.objectIndex].getMaterialIndex();
			assert(mat_id < scene.materials.size());
			const auto &material = scene.materials[mat_id];
			scene.fillHitInfo(refractedHit, refractedRay, material->smooth);
			refractionColor = material->shade(refractedHit, refractedRay, scene)._xyz();
		} else {
			refractionColor = scene.backgroundColor.xyz();
		}
		color = mix(reflectionColor, refractionColor, 1.0f - fresnel);
	} else {
		color = reflectionColor;
	}

	// return vec4(fresnel, fresnel, fresnel, 1.0f);
	// if (dotNormal > 0.001f) color = correctedNormal;
	return vec4(color * this->albedo, 1.0f);
}
