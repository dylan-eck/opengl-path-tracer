#version 430 core

in vec2 uv;

out vec4 fragColor;

uniform int numBounces;
uniform int numSamples;

struct Material {
    vec3 color;
    float emission_strength;
    vec3 emission_color;
    float padding;
};

struct Sphere {
    vec3 position;
    float radius;
    Material material;
};

struct Plane {
    vec3 position;
    float padding0;
    vec3 normal;
    float padding1;
    Material material;
};

struct Rectangle {
    vec3 position;
    float padding0;
    vec3 normal;
    float padding1;
    vec3 u;
    float padding2;
    vec3 v;
    Material material;
};

layout (binding = 0) buffer sphereBuffer {
    Sphere spheres[];
};

layout (binding = 1) buffer planeBuffer {
    Plane planes[];
};

layout (binding = 2) buffer rectBuffer {
    Rectangle rects[];
};

vec3 cameraPosition = vec3(0.0, 0.0, -4.0);
vec3 cameraTarget = vec3(0.0, 0.0, 0.0);
vec3 cameraUp = vec3(0.0, 1.0, 0.0);

float fov = 0.8;
float nearClip = 0.1;
float aspect = 1.0;

const highp float PI = 3.1415926;

uint pcg_hash(uint x) {
    uint state = x * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

uint state;
float random() {
    state ^= (state << 13u);
    state ^= (state >> 17u);
    state ^= (state << 5u);
    return float(state) / float(uint(-1));
}

float normal_random() {
    float theta = 2.0 * PI * random();
    float rho = sqrt(-2.0 * log(random()));
    return rho * cos(theta);
}

vec3 random_direction() {
    float x = normal_random();
    float y = normal_random();
    float z = normal_random();
    return normalize(vec3(x, y, z));
}

vec3 random_direction_hemisphere(vec3 normal) {
    vec3 direction = random_direction();
    return direction * sign(dot(normal, direction));
}

struct Ray {
    vec3 origin;
    vec3 direction;
};

struct IntersectionInfo {
    bool isIntersecting;
    float dist;
    vec3 location;
    vec3 normal;
    Material material;
};

IntersectionInfo SphereIntersection(Ray ray, vec3 center, float radius) {
    IntersectionInfo info;
    info.isIntersecting = false;

    vec3 offset_ray_origin = ray.origin - center;
    float a = dot(ray.direction, ray.direction);
    float b = 2 * dot(offset_ray_origin, ray.direction);
    float c = dot(offset_ray_origin, offset_ray_origin) - radius * radius;
    float discrminant = b * b - 4 * a * c;

    if (discrminant >= 0) {
        float dst = (-b - sqrt(discrminant)) / (2.0 * a);

        if (dst >= 0) {
            info.isIntersecting = true;
            info.dist = dst;
            info.location = ray.origin + ray.direction * dst;
            info.normal = normalize(info.location - center);
        }
    }
    return info;
}

IntersectionInfo PlaneIntersection(Ray ray, vec3 position, vec3 normal) {
    IntersectionInfo info;
    info.isIntersecting = false;

    float denom = dot(ray.direction, normal);
    if (abs(denom) > 1e-6) {
        float t = dot(position - ray.origin, normal) / denom;

        if (t >= 0) {
            info.isIntersecting = true;
            info.dist = t;
            info.location = ray.origin + ray.direction * t;
            info.normal = normalize(normal);
        }
    }
    return info;
}

IntersectionInfo RectangleIntersection(Ray ray, Rectangle rect) {
    IntersectionInfo info;
    info.isIntersecting = false;

    IntersectionInfo pinfo = PlaneIntersection(ray, rect.position, rect.normal);

    if (pinfo.isIntersecting) {
        vec3 rel_intersect = pinfo.location - rect.position;

        vec3 u_normalized = normalize(rect.u);
        vec3 v_normalized = normalize(rect.v);

        float proj_u = dot(rel_intersect, u_normalized);
        float proj_v = dot(rel_intersect, v_normalized);

        float half_width = length(rect.u) / 2.0;
        float half_height = length(rect.v) / 2.0;
        if (abs(proj_u) <= half_width && abs(proj_v) <= half_height) {
            info = pinfo;
            info.material = rect.material;
        }
    }

    return info;
}

const int NUM_RECTANGLES = 1;
Rectangle rectangles[NUM_RECTANGLES];

IntersectionInfo findNearestIntersection(Ray ray) {
    IntersectionInfo info;
    info.isIntersecting = false;
    info.dist = float(uint(-1));
    info.material.color = vec3(0.0);

    for (int i = 0; i < spheres.length(); i++) {
        Sphere sphere = spheres[i];
        IntersectionInfo sinfo = SphereIntersection(
            ray, sphere.position, sphere.radius);

        if (sinfo.isIntersecting && (sinfo.dist < info.dist)) {
            info = sinfo;
            info.material = sphere.material;
        }
    }

    for (int i = 0; i < planes.length(); i++) {
        Plane plane = planes[i];
        IntersectionInfo pinfo = PlaneIntersection(ray, plane.position, plane.normal);

        if (pinfo.isIntersecting && (pinfo.dist < info.dist)) {
            info = pinfo;
            info.material = plane.material;
        }
    }

    for (int i = 0; i < NUM_RECTANGLES; i++) {
        Rectangle rectangle = rectangles[i];
        IntersectionInfo rinfo = RectangleIntersection(ray, rectangle);

        if (rinfo.isIntersecting && (rinfo.dist < info.dist)) {
            info = rinfo;
        }
    }

    return info;
}

const float P = 1.0 / (2.0 * PI);
vec3 tracePath(Ray ray) {
    vec3 pixelColor = vec3(0.0);
    vec3 rayColor = vec3(1.0);
    float epsilon = 1e-4;

    for (int i = 0; i <= numBounces; i++) {
        IntersectionInfo info = findNearestIntersection(ray);
        if (!info.isIntersecting) break;

        vec3 emission = info.material.emission_color * info.material.emission_strength;
        pixelColor += emission * rayColor;

        ray.origin = info.location + epsilon * info.normal;
        ray.direction = random_direction_hemisphere(info.normal);

        float cos_theta = dot(ray.direction, info.normal);
        vec3 BRDF = info.material.color / PI;

        rayColor *= BRDF * cos_theta / P;
    }

    return pixelColor;
}

void main() {
    vec2 screen_uv = (uv - 0.5) * 2.0;

    float half_height = nearClip * tan(fov / 2.0);
    float half_width = aspect * half_height;

    vec3 forward = normalize(cameraTarget - cameraPosition);
    vec3 right = normalize(cross(forward, cameraUp));
    vec3 adj_up = normalize(cross(right, forward));

    rectangles[0].position = cameraPosition + vec3(0.0, half_height - 0.0001, 0.125);
    rectangles[0].normal = vec3(0.0, -1.0, 0.0);
    rectangles[0].u = vec3(0.06, 0.0, 0.0);
    rectangles[0].v = vec3(0.0, 0.0, 0.06);
    rectangles[0].material.color = vec3(0.0);
    rectangles[0].material.emission_color = vec3(1.0);
    rectangles[0].material.emission_strength = 3.0;

    Ray ray;
    ray.origin = cameraPosition;
    ray.direction = (
        half_width * screen_uv.x * right +
        half_height * screen_uv.y * adj_up +
        nearClip * forward
    );
    ray.direction = normalize(ray.direction);



    uint seed = uint(gl_FragCoord.x) * 83094u + uint(gl_FragCoord.y) * 275481u;
    state = pcg_hash(seed);

    vec3 color = vec3(0.0);
    for (int i = 0; i < numSamples; i++) {
        color += tracePath(ray);
    }
    color /= float(numSamples);

    fragColor = vec4(color, 1.0);
}