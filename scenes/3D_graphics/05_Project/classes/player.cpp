#include "player.hpp"

using namespace vcl;

void Player::setup(float scale_, std::map<std::string,GLuint>& shaders, Grid& g_) {

    // Scaling
    scale = scale_;

    size *= scale;
    head_x *= scale;
    head_y *= scale;
    head_z *= scale;
    body_y *= scale;
    body_z *= scale;
    leg_y *= scale;
    leg_z *= scale;
    arm_y *= scale;
    arm_z *=  scale;
    dist_feet = leg_z + g.step / 2.0f;
    dist_head = body_z + head_z;

    bool find_start_place = false;

    g = g_;

    distance = 1.2f * scale;
    error = leg_z / 10.0f;
    v = {0, 0, 0};

    std::uniform_int_distribution<int> start_x(0, g.Nx);
    std::uniform_int_distribution<int> start_y(0, g.Ny);
    std::default_random_engine generator;
    generator.seed(5);
    create_player();

    x = start_x(generator);
    y = start_y(generator);

    while (!find_start_place){

        z = g.surface_z[y][x];
        if (g.blocks[z+1][y][x] == 0 && g.blocks[z+2][y][x] == 0 && g.blocks[z+3][y][x] == 0 && g.blocks[z+4][y][x] == 0)
            find_start_place = true;
        else
            z += 1;
    }
    p = g.blocks_to_position(x, y, z) + vec3{0, 0, dist_feet};

    angle = 0;
    camera_angle = 0;
    timer.scale = 0.5f;
    // Set the same shader for all the elements
    hierarchy.set_shader_for_all_elements(shaders["mesh"]);

    // Initialize helper structure to display the hierarchy skeleton
    hierarchy_visual_debug.init(shaders["segment_im"], shaders["mesh"]);

    hierarchy["body"].transform.translation = p;
}

void Player::frame_draw(std::map<std::string, GLuint> &shaders, scene_structure &scene, gui_scene_structure gui_scene) {

    timer.update();
    updatePosition(scene);

    scene.camera.translation = -hierarchy["body"].transform.translation;

    // The default display
    if(gui_scene.surface) {
        glBindTexture(GL_TEXTURE_2D, player_texture);
        draw(hierarchy, scene.camera, shaders["mesh"]);
        glBindTexture(GL_TEXTURE_2D, scene.texture_white);
    }

    hierarchy.update_local_to_global_coordinates();
    if(gui_scene.wireframe) // Display the hierarchy as wireframe
        draw(hierarchy, scene.camera, shaders["wireframe"]);

    if(gui_scene.skeleton) // Display the skeleton of the hierarchy (debug)
        hierarchy_visual_debug.draw(hierarchy, scene.camera);
}

void Player::keyboard_input(scene_structure &scene, GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (!falling) {
        moving_up = (glfwGetKey(window, GLFW_KEY_W));
        moving_down = (glfwGetKey(window, GLFW_KEY_S));
        jumping = (glfwGetKey(window, GLFW_KEY_SPACE));
    }
    moving_left = (glfwGetKey(window, GLFW_KEY_A));
    moving_right = (glfwGetKey(window, GLFW_KEY_D));

    moving_camera_up = (glfwGetKey(window, GLFW_KEY_UP));
    moving_camera_down = (glfwGetKey(window, GLFW_KEY_DOWN));
    moving_camera_left = (glfwGetKey(window, GLFW_KEY_LEFT));
    moving_camera_right = (glfwGetKey(window, GLFW_KEY_RIGHT));
}

void Player::updatePosition(scene_structure &scene) {
    const float t = timer.t;

    moving = moving_up || moving_down || jumping || falling;
    bool turning = moving_left || moving_right;

    float speed = g.step / 2;
    float speed_turn = M_PI / 10;
    float speed_turn_camera = M_PI / 10;

    if (moving || turning) {

        mat3 const Symmetry = {-1, 0, 0, 0, 1, 0, 0, 0, 1};

        mat3 const R_arm = rotation_from_axis_angle_mat3({0, 1, 0}, 0.4f * std::sin(2 * 3.14f * (2 * t - 0.4f)));
        mat3 const R_leg = rotation_from_axis_angle_mat3({0, 1, 0}, 0.8f * std::sin(2 * 3.14f * (2 * t - 0.4f)));
        mat3 const R_head = rotation_from_axis_angle_mat3({0, 0, 1}, 0.3f * std::sin(2 * 3.14f * (t - 0.6f)));

        hierarchy["mov_head"].transform.rotation = R_head;

        hierarchy["mov_leg_left"].transform.rotation = Symmetry * R_leg;
        hierarchy["mov_leg_right"].transform.rotation = R_leg;

        hierarchy["mov_arm_right"].transform.rotation = R_arm;
        hierarchy["mov_arm_left"].transform.rotation = Symmetry * R_arm;

        if (jumping)
            jump(scale * 0.8f);
        else if (falling)
            fall(scale * 80.0f);
        if (moving)
            move(speed);
        if (turning)
            turn(speed_turn);
    }else{

        mat3 const R_arm = rotation_from_axis_angle_mat3({0, 1, 0}, 0);
        mat3 const R_leg = rotation_from_axis_angle_mat3({0, 1, 0}, 0);
        mat3 const R_head = rotation_from_axis_angle_mat3({0, 0, 1}, 0);

        hierarchy["mov_head"].transform.rotation = R_head;

        hierarchy["mov_leg_left"].transform.rotation = R_leg;
        hierarchy["mov_leg_right"].transform.rotation = R_leg;

        hierarchy["mov_arm_right"].transform.rotation = R_arm;
        hierarchy["mov_arm_left"].transform.rotation = R_arm;

        z =  (int) ((p.z-leg_z +g.step/2.0f - error) / g.step);
        p.z = (float) z*g.step + leg_z + g.step/2.0f;

        hierarchy["body"].transform.translation = p;
        v = vec3{0, 0,0};
    }

    if (moving_left) {
        camera_angle += speed_turn_camera;
        scene.camera.apply_rotation(speed_turn_camera, 0, 0, 0);
    } else if (moving_right) {
        camera_angle -= speed_turn_camera;
        scene.camera.apply_rotation(-speed_turn_camera, 0, 0, 0);
    }

    if (moving_camera_up) {
        scene.camera.apply_rotation(0, 0.01f, 0, 0);
    } else if (moving_camera_down) {
        scene.camera.apply_rotation(0, -0.01f, 0, 0);
    }

    if (moving_camera_right) {
        camera_angle -= 0.05f;
        scene.camera.apply_rotation(0.05f, 0, 0, 0);
    } else if (moving_camera_left) {
        camera_angle += 0.05f;
        scene.camera.apply_rotation(-0.05f, 0, 0, 0);
    }

}

void Player::jump(float initial_speed)
{

    v = v + vec3({0, 0, initial_speed});
    falling = true;
    jumping = false;
}


void Player::fall(float m)
{
    v = v - vec3{0, 0, g.step/4.0f};
}


void Player::move(float speed)
{

    bool up = check_up();
    bool down = check_down();
    bool ahead = check_ahead();
    bool behind = check_behind();

    if (moving_up){
        if(ahead){
            v.x = 0;
            v.y = 0;
        }else{
            v.x = speed * (float)cos(angle);
            v.y = speed * (float)-sin(angle);
        }
    }

    if (moving_down){
        if(behind){
            v.x = 0;
            v.y = 0;
        }else{
            v.x = speed*(float)-cos(angle);
            v.y = speed*(float) sin(angle);
        }
    }

    if (falling){
        if (down && v.z < 0){
            v.z = 0;
            falling = false;
        }
        if (up && v.z > 0){
            v.z = 0;
        }
    }

    p = p + v;
    if (down){
        z =  (int) ((p.z-leg_z +g.step/2.0f - error) / g.step);
        p.z = (float) z*g.step + leg_z + g.step/2.0f;
    }

    hierarchy["body"].transform.translation = p;

    if (!falling && !down){
        falling = true;
    }

}

void Player::turn(float speed)
{

    if (moving_left)
        angle -= speed;
    if (moving_right)
        angle += speed;
    hierarchy["body"].transform.rotation = rotation_from_axis_angle_mat3({0,0,-1}, angle);
}

void Player::create_player()
{

    // draw body
    mesh head = mesh_primitive_parallelepiped({0,0,0},{head_x,0,0},
                                              {0,head_y,0},{0,0,head_z});
    mesh body = mesh_primitive_parallelepiped({0,0,0},{size,0,0},
                                              {0,body_y,0},{0,0,body_z});
    mesh leg = mesh_primitive_parallelepiped({0,0,0},{size,0,0},
                                             {0,leg_y,0},{0,0,leg_z});
    mesh arm = mesh_primitive_parallelepiped({0,0,0},{size,0,0},
                                             {0,arm_y,0},{0,0,arm_z});
    mesh mov = mesh_primitive_sphere(scale * 0.01f);

    // Add texture
    const float x_a = 0.5f/4.0f;
    const float x_b = 0.5f/8.0f;

    const float y_a = 0.5f/2.0f;
    const float y_b = 0.5f/4.0f;

    // Texture
    player_texture =  create_texture_gpu(image_load_png("scenes/3D_graphics/05_Project/texture/player.png"),
                                         GL_REPEAT, GL_REPEAT );

    leg.texture_uv = {
            {1*x_a,2*y_a+y_b}, {1*x_a,2*y_a}, {1*x_a+x_b,2*y_a}, {1*x_a+x_b,2*y_a+y_b},
            {x_b,4*y_a}, {x_b,2*y_a+y_b}, {x_a,2*y_a+y_b}, {x_a,4*y_a},
            {x_a,4*y_a}, {x_a,2*y_a+y_b}, {x_a+x_b,2*y_a+y_b}, {x_a+x_b,4*y_a},
            {2*x_a,4*y_a}, {2*x_a,2*y_a+y_b}, {x_a+x_b,2*y_a+y_b}, {x_a+x_b,4*y_a},
            {0,4*y_a}, {0,2*y_a+y_b}, {x_b,2*y_a+y_b}, {x_b,4*y_a},
            {1*x_b,2*y_a+y_b}, {1*x_b,2*y_a}, {1*x_a,2*y_a}, {1*x_a,2*y_a+y_b},
    };

    arm.texture_uv = {
            {6*x_a,2*y_a+y_b}, {6*x_a,2*y_a}, {6*x_a+x_b,2*y_a}, {6*x_a+x_b,2*y_a+y_b},
            {5*x_a+x_b,4*y_a}, {5*x_a+x_b,2*y_a+y_b}, {6*x_a,2*y_a+y_b}, {6*x_a,4*y_a},
            {6*x_a,4*y_a}, {6*x_a,2*y_a+y_b}, {6*x_a+x_b,2*y_a+y_b}, {6*x_a+x_b,4*y_a},
            {7*x_a,4*y_a}, {7*x_a,2*y_a+y_b}, {6*x_a+x_b,2*y_a+y_b}, {6*x_a+x_b,4*y_a},
            {5*x_a,4*y_a}, {5*x_a,2*y_a+y_b}, {5*x_a+x_b,2*y_a+y_b}, {5*x_a+x_b,4*y_a},
            {5*x_a+1*x_b,2*y_a+y_b}, {5*x_a+1*x_b,2*y_a}, {6*x_a,2*y_a}, {6*x_a,2*y_a+y_b},
    };

    body.texture_uv = {
            {3*x_a+x_b,2*y_a+y_b}, {3*x_a+x_b,2*y_a}, {4*x_a+x_b,2*y_a}, {4*x_a+x_b,2*y_a+y_b},
            {2*x_a+x_b,4*y_a}, {2*x_a+x_b,2*y_a+y_b}, {3*x_a+x_b,2*y_a+y_b}, {3*x_a+x_b,4*y_a},
            {3*x_a+x_b,4*y_a}, {3*x_a+x_b,2*y_a+y_b}, {4*x_a,2*y_a+y_b}, {4*x_a,4*y_a},
            {5*x_a,4*y_a}, {5*x_a,2*y_a+y_b}, {4*x_a,2*y_a+y_b}, {4*x_a,4*y_a},
            {2*x_a,4*y_a}, {2*x_a,2*y_a+y_b}, {2*x_a+x_b,2*y_a+y_b}, {2*x_a+x_b,4*y_a},
            {2*x_a+x_b,2*y_a+y_b}, {2*x_a+x_b,2*y_a}, {3*x_a+x_b,2*y_a}, {3*x_a+x_b,2*y_a+y_b},
    };

    head.texture_uv = {
            {2*x_a,1*y_a}, {2*x_a,0}, {3*x_a,0}, {3*x_a,1*y_a},
            {1*x_a,2*y_a}, {1*x_a,1*y_a}, {2*x_a,1*y_a}, {2*x_a,2*y_a},
            {2*x_a,2*y_a}, {2*x_a,1*y_a}, {3*x_a,1*y_a}, {3*x_a,2*y_a},
            {4*x_a,2*y_a}, {4*x_a,1*y_a}, {3*x_a,1*y_a}, {3*x_a,2*y_a},
            {0,2*y_a}, {0,1*y_a}, {1*x_a,1*y_a}, {1*x_a,2*y_a},
            {1*x_a,1*y_a}, {1*x_a,0}, {2*x_a,0}, {1*x_a,1*y_a},
    };


    hierarchy.add(body, "body");

    hierarchy.add(mov, "mov_head", "body", {head_x/2.0f,head_y/2.0f,body_z});
    hierarchy.add(mov, "mov_leg_right", "body", {size/2.0f,body_y-leg_y/2.0f,0});
    hierarchy.add(mov, "mov_leg_left", "body", {size/2.0f,body_y/2.0f-leg_y/2.0f,0});
    hierarchy.add(mov, "mov_arm_right", "body", {size/2.0f,-arm_y/2.0f,body_z});
    hierarchy.add(mov, "mov_arm_left", "body", {size/2.0f,body_y + arm_y/2.0f,body_z});

    hierarchy.add(head, "head", "mov_head", {-head_x/2.0f-size/4.0f,-head_y/2.0f, 0});

    hierarchy.add(leg, "leg_right", "mov_leg_right", {-size/2.0f, -leg_y/2.0f, -leg_z});
    hierarchy.add(leg, "leg_left", "mov_leg_left", {-size/2.0f, -leg_y/2.0f, -leg_z});

    hierarchy.add(arm, "arm_right", "mov_arm_right", {-size/2.0f, -arm_y/2.0f, -arm_z});
    hierarchy.add(arm, "arm_left", "mov_arm_left", {-size/2.0f, -arm_y/2.0f, -arm_z});
}

int Player::block_down_player()
{
    return g.blocks[z][y][x];
}

int Player::block_down(vec3 p)
{

    vec3 p2 = p + vec3{0, 0, 0};
    return g.position_to_block(p2);
}

int Player::block_up(vec3 p)
{
    float d = sqrt (size * size /4.0f + body_y * body_y/4.0f);
    vec3 p2 = p + vec3{d*(float) cos(angle),d*(float)-sin(angle), dist_feet};

    return g.position_to_block(p2);
}

bool Player::check_ahead()
{
    //std::cout << "ahead debug" << std::endl;

    float d = sqrt(size * size / 4.0f + body_y * body_y / 4.0f);
    //std::cout << "d = " << d << std::endl;
    float teta = atan((size/2.0f)/(body_y/2.0f));
    //std::cout << "teta + angle = " << teta  + angle << std::endl;
    vec3 center = p + vec3{d * (float)sin(angle+teta),d*(float)cos(angle+teta), 0 };
    vec3 p1 = center + vec3{0, 0, -leg_z +g.step/2.0f + error};
    vec3 p2 = center + vec3{0, 0, 0};
    vec3 p3 = center + vec3{0, 0, dist_head - error};
    //std::cout << "p = " << p << std::endl;
    //std::cout << "center = " << center << std::endl;
    //std::cout << "p1 = " <<  p1 << std::endl;
    //std::cout << "p2 = " <<  p2 << std::endl;
    //std::cout << "p3 = " <<  p3 << std::endl;
    //std::cout << "step = " << g.step << std::endl;
    //std::cout << "z standing = " << (int) (p1[2] / g.step) << std::endl;
    vec3 ahead = vec3{ distance * (float) cos(angle), distance * (float)-sin(angle), 0};
    //std::cout << "leg_z = " << leg_z << std::endl;
    //std::cout << g.position_to_block(p1 + ahead) << std::endl;
    bool l = g.position_to_block(p1 + ahead) != 0 || g.position_to_block(p2 + ahead) != 0 || g.position_to_block(p3 + ahead) != 0;
    return l;
}

bool Player::check_behind()
{
    float d = sqrt(size * size / 4.0f + body_y * body_y / 4.0f);
    float teta = atan((size/2.0f)/(body_y/2.0f));
    vec3 center = p + vec3{d * (float)cos(M_PI/2.0f-angle-teta),d*(float)-sin(M_PI/2.0f - angle-teta), 0 };
    vec3 p1 = center + vec3{0, 0, -leg_z +g.step/2.0f + error};
    vec3 p2 = center + vec3{0, 0, 0};
    vec3 p3 = center + vec3{0, 0, dist_head - error};
    vec3 behind = vec3{ distance * (float)-cos(angle), distance * (float)sin(angle), 0};
    bool l = g.position_to_block(p1 + behind) != 0 || g.position_to_block(p2 + behind) != 0 || g.position_to_block(p3 + behind) != 0;
    return l;
}

bool Player::check_down()
{
    //std::cout << "down debug" << std::endl;
    float d = sqrt(size * size / 4.0f + body_y * body_y / 4.0f);
    float teta = atan((size/2.0f)/(body_y/2.0f));
    vec3 center = p + vec3{d * (float)cos(M_PI/2.0f-angle-teta),d*(float)-sin(M_PI/2.0f - angle-teta), 0 };
    vec3 p1 = center + vec3{0, 0, -leg_z +g.step/2.0f - error};
    //std::cout << "p = " << p << std::endl;
    //std::cout << "center = " << center << std::endl;
    //std::cout << "p1 = " <<  p1 << std::endl;
    //std::cout << "p2 = " <<  p2 << std::endl;
    //std::cout << "p3 = " <<  p3 << std::endl;
    //std::cout << "step = " << g.step << std::endl;
    //std::cout << "z standing = " << (p1[2] / g.step) << std::endl;
    //std::cout << "leg_z = " << leg_z << std::endl;
    //std::cout << g.position_to_block(p1) << std::endl;
    bool l = g.position_to_block(p1) != 0;
    return l;
}

bool Player::check_up()
{
    float d = sqrt(size * size / 4.0f + body_y * body_y / 4.0f);
    float teta = atan((size/2.0f)/(body_y/2.0f));
    vec3 center = p + vec3{d * (float)cos(M_PI/2.0f-angle-teta),d*(float)-sin(M_PI/2.0f - angle-teta), 0 };
    vec3 p1 = center + vec3{0, 0, dist_head + error};
    bool l = g.position_to_block(p1) != 0;
    return l;
}

