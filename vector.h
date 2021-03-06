/* 4x4 column major matrices, vectors and quaternions */

typedef float vec2[2];
typedef float vec3[3];
typedef float vec4[4];
typedef float mat4[16];

void mat_identity(mat4 m);
void mat_copy(mat4 p, const mat4 m);
void mat_mix(mat4 m, const mat4 a, const mat4 b, float v);
void mat_mul(mat4 m, const mat4 a, const mat4 b);
void mat_mul44(mat4 m, const mat4 a, const mat4 b);
void mat_frustum(mat4 m, float left, float right, float bottom, float top, float n, float f);
void mat_perspective(mat4 m, float fov, float aspect, float near, float far);
void mat_look(mat4 m, const vec3 origin, const vec3 forward, const vec3 up);
void mat_look_at(mat4 m, const vec3 eye, const vec3 center, const vec3 up);
void mat_ortho(mat4 m, float left, float right, float bottom, float top, float n, float f);
void mat_scale(mat4 m, float x, float y, float z);
void mat_rotate_x(mat4 m, float angle);
void mat_rotate_y(mat4 m, float angle);
void mat_rotate_z(mat4 m, float angle);
void mat_translate(mat4 m, float x, float y, float z);
void mat_transpose(mat4 to, const mat4 from);
void mat_invert(mat4 out, const mat4 m);

void mat_vec_mul(vec3 p, const mat4 m, const vec3 v);
void mat_vec_mul_n(vec3 p, const mat4 m, const vec3 v);
void mat_vec_mul_t(vec3 p, const mat4 m, const vec3 v);
void vec_init(vec3 p, float x, float y, float z);
void vec_scale(vec3 p, const vec3 v, float s);
void vec_add(vec3 p, const vec3 a, const vec3 b);
void vec_sub(vec3 p, const vec3 a, const vec3 b);
void vec_mul(vec3 p, const vec3 a, const vec3 b);
void vec_div(vec3 p, const vec3 a, const vec3 b);
void vec_div_s(vec3 p, const vec3 a, float b);
void vec_lerp(vec3 p, const vec3 a, const vec3 b, float t);
void vec_average(vec3 p, const vec3 a, const vec3 b);
void vec_cross(vec3 p, const vec3 a, const vec3 b);
float vec_dot(const vec3 a, const vec3 b);
float vec_length(const vec3 a);
float vec_dist2(const vec3 a, const vec3 b);
float vec_dist(const vec3 a, const vec3 b);
void vec_normalize(vec3 v, const vec3 a);
void vec_face_normal(vec3 n, const vec3 p0, const vec3 p1, const vec3 p2);
void vec_negate(vec3 p, const vec3 a);
void vec_invert(vec3 p, const vec3 a);
void vec_yup_to_zup(vec3 v);

void quat_init(vec4 p, float x, float y, float z, float w);
float quat_dot(const vec4 a, const vec4 b);
void quat_invert(vec4 out, const vec4 q);
void quat_copy(vec4 out, const vec4 q);
void quat_conjugate(vec4 out, const vec4 q);
void quat_mul(vec4 q, const vec4 a, const vec4 b);
void quat_normalize(vec4 q, const vec4 a);
void quat_lerp(vec4 p, const vec4 a, const vec4 b, float t);
void quat_lerp_normalize(vec4 p, const vec4 a, const vec4 b, float t);
void quat_lerp_neighbor_normalize(vec4 p, const vec4 a, const vec4 b, float t);
void mat_from_quat(mat4 m, const vec4 q);
void mat_from_pose(mat4 m, const vec3 t, const vec4 q, const vec3 s);
void quat_from_mat(vec4 q, const mat4 m);
int mat_is_negative(const mat4 m);
void mat_decompose(const mat4 m, vec3 t, vec4 q, vec3 s);
