
#include <functional>
#include <materials.hpp>
#include <scene.hpp>
#include <util/utils.hpp>
#include <sample.hpp>

const float EPS		  = 0.001f;
const int	MAX_DEPTH = 3;

DiffuseMaterial::DiffuseMaterial(const JSONObject &obj, const Scene &scene) : Material(obj, true, true) {
	const auto &albedoJSON = obj["albedo"];
	if (albedoJSON.is<JSONArray>()) {
		const auto &colorJSON = albedoJSON.as<JSONArray>();
		this->albedoColor =
			vec3{colorJSON[0].as<JSONNumber>(), colorJSON[1].as<JSONNumber>(), colorJSON[2].as<JSONNumber>()};
		this->albedo = nullptr;
		return;
	} else if (albedoJSON.is<JSONString>()) {
		this->albedoColor			 = vec3(0, 0, 0);
		std::string_view textureName = obj["albedo"].as<JSONString>();
		albedo						 = scene.getTexture(textureName);
	} else {
		throw std::runtime_error("Invalid albedo type for DiffuseMaterial");
	}
	doubleSided = false;
}

vec3 cosWeightedHemissphereDir(vec3 normal, uint32_t &seed) {
	float z = randomFloat(seed) * 2.0 - 1.0;
	float a = randomFloat(seed) * 2.0 * M_PI;
	float r = sqrt(1.0 - z * z);
	float x = r * cos(a);
	float y = r * sin(a);

	// Convert unit vector in sphere to a cosine weighted vector in hemissphere
	auto res = normal + vec3(x, y, z);
	if (res.x > 0.001f || res.y > 0.001f || res.z > 0.001f) res = normalize(res);
	else res = normal;
	return res;
}

vec4 DiffuseMaterial::shade(const RayHit &hit, const Ray &, const Scene &scene, uint32_t &seed) const {
	if (hit.depth >= MAX_DEPTH) { return scene.backgroundColor; }
	vec3 color = 0;
	//return vec4(hit.pos, 1.f);
	for (const auto &light : scene.lights) {
		vec3  lightDir	 = light.position - hit.pos;
		float distanceSq = lengthSquared(lightDir);
		float distance	 = std::sqrt(distanceSq);
		lightDir /= distance;

		if (this->receivesShadows) {
			auto shadowHit = scene.intersect(Ray(hit.pos + hit.normal * EPS, lightDir, Ray::Type::Shadow));
			if (shadowHit.t > EPS && shadowHit.t * shadowHit.t < distanceSq - EPS) {
				continue;	  // shadow
			}
		}

		color += light.color * light.intensity * std::max(0.f, dot(hit.normal, lightDir)) / (4.f * M_PIf * distanceSq);
	}

	//auto terminationP = (1.f - max(color)) / 2.f;
	//auto diceroll = randomFloat(seed);
	//if( diceroll < terminationP ) {
	//	return vec4(color, 1.0f); // russian roulette
	//}

	vec3   randomDir = cosWeightedHemissphereDir(hit.normal, seed);
	Ray	   reflectedRay(hit.pos + randomDir * EPS, randomDir);
	RayHit reflectedHit = scene.intersect(reflectedRay);
	if (reflectedHit.objectIndex != -1u) {
		reflectedHit.depth	 = hit.depth + 1;
		const auto	mat_id	 = scene.getObjects()[reflectedHit.objectIndex]->getMaterialIndex();
		const auto &material = scene.materials[mat_id];
		scene.fillHitInfo(reflectedHit, reflectedRay, material->smooth);
		color +=
			material->shade(reflectedHit, reflectedRay, scene, seed)._xyz() * std::max(0.f, dot(hit.normal, randomDir));
	} else {
		color += scene.backgroundColor.xyz();
	}

	if (this->albedo) {
		color *= this->albedo->sample(hit);
	} else {
		color *= this->albedoColor;
	}

	//color *= 1.f / (1.f - terminationP); // russian roulette probability
	return vec4(color, 1.0f);
}

vec4 ReflectiveMaterial::shade(const RayHit &hit, const Ray &ray, const Scene &scene, uint32_t &seed) const {
	if (hit.depth >= MAX_DEPTH) { return scene.backgroundColor; }
	vec3   color		= 0;
	vec3   reflectedDir = normalize(reflect(ray.direction, hit.normal));
	Ray	   reflectedRay(hit.pos + hit.normal * EPS, reflectedDir);
	RayHit reflectedHit = scene.intersect(reflectedRay);
	if (reflectedHit.objectIndex != -1u) {
		reflectedHit.depth	 = hit.depth + 1;
		const auto	mat_id	 = scene.getObjects()[reflectedHit.objectIndex]->getMaterialIndex();
		const auto &material = scene.materials[mat_id];
		scene.fillHitInfo(reflectedHit, reflectedRay, material->smooth);
		color = material->shade(reflectedHit, reflectedRay, scene, seed).xyz();
	} else {
		color = scene.backgroundColor.xyz();
	}
	return vec4(color * this->albedo, 1.0f);
}

static inline float F0(float ior1, float ior2) {
	float f = (ior1 - ior2) / (ior1 + ior2);
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

vec4 RefractiveMaterial::shade(const RayHit &hit, const Ray &ray, const Scene &scene, uint32_t &seed) const {
	if (hit.depth >= MAX_DEPTH) { return scene.backgroundColor; }
	vec3  color		 = 0;
	float dotNormal	 = dot(hit.normal, ray.direction);
	bool  isEntering = dotNormal < 0.0f;
	vec3  normal	 = hit.normal;

	float ior1 = 1.0f;
	float ior2 = this->ior;

	if (!isEntering) {
		std::swap(ior1, ior2);
		dotNormal = -dotNormal;
		normal	  = -normal;
	}

	float eta		= ior1 / ior2;	   // Snell's law
	vec3  normalEPS = normal * EPS;

	Ray reflectedRay(hit.pos + normalEPS, normalize(reflect(ray.direction, normal)));
	Ray refractedRay(hit.pos - normalEPS, refract(ray.direction, normal, eta));
	if (refractedRay.direction != vec3(0.f)) refractedRay.direction = normalize(refractedRay.direction);
	if (isnan(refractedRay.direction)) {
		dbLog(dbg::LOG_ERROR, ray.direction, normal, eta, refract(ray.direction, normal, eta));
		// assert(false);
	}

	float f0	  = F0(ior1, ior2);
	float fresnel = fresnelReflectAmount(ior1, ior2, normal, ray.direction, f0, 1.0f);

	float randomSelect = randomFloat(seed);
	
	// importance sampling the fresnel term
	if (randomSelect < fresnel) {
		RayHit reflectedHit = scene.intersect(reflectedRay);
		if (reflectedHit.objectIndex != -1u) {
			reflectedHit.depth = hit.depth + 1;
			const auto mat_id  = scene.getObjects()[reflectedHit.objectIndex]->getMaterialIndex();
			assert(mat_id < scene.materials.size());
			const auto &material = scene.materials[mat_id];
			scene.fillHitInfo(reflectedHit, reflectedRay, material->smooth);
			color = material->shade(reflectedHit, reflectedRay, scene, seed)._xyz();
		} else {
			color = scene.backgroundColor.xyz();
		}
	} else {
		RayHit refractedHit = scene.intersect(refractedRay);
		if (refractedHit.objectIndex != -1u) {
			refractedHit.depth = hit.depth + 1;
			const auto mat_id  = scene.getObjects()[refractedHit.objectIndex]->getMaterialIndex();
			assert(mat_id < scene.materials.size());
			const auto &material = scene.materials[mat_id];
			scene.fillHitInfo(refractedHit, refractedRay, material->smooth);
			color = material->shade(refractedHit, refractedRay, scene, seed)._xyz();
		} else {
			color = scene.backgroundColor.xyz();
		}
	}
	if(!isEntering) {
		color *= exp(this->absorbtion * -hit.t);	 // Attenuate color for exiting the object
	}

	return vec4(color, 1.0f);
}
