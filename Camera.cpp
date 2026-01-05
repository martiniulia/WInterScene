#include "Camera.hpp"
#include <glm/gtc/matrix_transform.hpp>

namespace gps {

    Camera::Camera(glm::vec3 cameraPosition,
        glm::vec3 cameraTarget,
        glm::vec3 cameraUp) {

        this->cameraPosition = cameraPosition;
        this->cameraTarget = cameraTarget;

        cameraFrontDirection = glm::normalize(cameraTarget - cameraPosition);
        cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, cameraUp));
        cameraUpDirection = glm::normalize(glm::cross(cameraRightDirection, cameraFrontDirection));
    }

    glm::mat4 Camera::getViewMatrix() {
        return glm::lookAt(
            cameraPosition,
            cameraPosition + cameraFrontDirection,
            cameraUpDirection
        );
    }

    glm::vec3 Camera::getPosition() const{
        return cameraPosition;
    }

    void Camera::move(MOVE_DIRECTION direction, float speed) {

        if (direction == MOVE_FORWARD)
            cameraPosition += cameraFrontDirection * speed;

        if (direction == MOVE_BACKWARD)
            cameraPosition -= cameraFrontDirection * speed;

        if (direction == MOVE_RIGHT)
            cameraPosition += cameraRightDirection * speed;

        if (direction == MOVE_LEFT)
            cameraPosition -= cameraRightDirection * speed;
    }

    void Camera::rotate(float pitch, float yaw) {

        glm::vec3 direction;
        direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        direction.y = sin(glm::radians(pitch));
        direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

        cameraFrontDirection = glm::normalize(direction);
        cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, glm::vec3(0.0f, 1.0f, 0.0f)));
        cameraUpDirection = glm::normalize(glm::cross(cameraRightDirection, cameraFrontDirection));
    }

    void Camera::setPosition(const glm::vec3& pos) {
        cameraPosition = pos;
    }


}
