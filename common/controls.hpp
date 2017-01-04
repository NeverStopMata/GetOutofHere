#ifndef CONTROLS_HPP
#define CONTROLS_HPP
void lightSwitch(bool * lightOn);
float getScanDistRT();
vec3 getCameraOrg();
glm::mat4 getViewMatrix();
glm::mat4 getProjectionMatrix();
glm::vec3 getViewDirection();
glm::quat getFrontRotation();
glm::mat4 getRotationMatrix_car();
glm::mat4 getModelMatrix_car();
glm::mat4 getModMtrxTyre1();
glm::mat4 getModMtrxTyre2();
glm::mat4 getModMtrxTyre3();
glm::mat4 getModMtrxTyre4();
glm::mat4 getModMtrxLightGun();
void computeMatricesFromInputs_my(std::vector<glm::vec3>  normal_surfaces, std::vector<glm::vec3>  vertices, ghost * man);
#endif