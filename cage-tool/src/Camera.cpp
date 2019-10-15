#include "Camera.h"

Camera::Camera() {
	eye = glm::vec3(0.0f,0.0f, 6.0f);
	up = glm::vec3(0.0f, 1.0f, 0.0f);
	centre = glm::vec3(0.0f, 0.0f, 0.0f);

	longitudeRotRad = 0;
	latitudeRotRad = 0;
}

Camera::~Camera() {
	// nothing to do here
}

// Returns view matrix for the camera
glm::mat4 Camera::getLookAt() {
	glm::vec3 eyeTemp = glm::rotateY(eye, -longitudeRotRad);
	eyeTemp = glm::rotate(eyeTemp, latitudeRotRad, glm::cross(eyeTemp, glm::vec3(0.0, 1.0, 0.0)));

	return glm::lookAt(eyeTemp, centre, up);
}

// Returns position of the camera
glm::vec3 Camera::getPosition() {
	glm::vec3 eyeTemp = glm::rotateY(eye, -longitudeRotRad);
	eyeTemp = glm::rotate(eyeTemp, latitudeRotRad, glm::cross(eyeTemp, glm::vec3(0.0, 1.0, 0.0)));

	return eyeTemp;
}

// Rotates camera along longitudinal axis (spherical coords)
void Camera::updateLongitudeRotation(float rad) {
	longitudeRotRad += rad * M_PI/180;
}

// Rotates camera along latitudinal axis (spherical coords)
void Camera::updateLatitudeRotation(float rad) {
	latitudeRotRad += rad * M_PI/180;
	if (latitudeRotRad > M_PI/2 - 0.01f) {
		latitudeRotRad = M_PI/2 - 0.01f;
	}
	else if (latitudeRotRad < -M_PI/2 + 0.01f) {
		latitudeRotRad = -M_PI/2 + 0.01f;
	}
}

// Update camera eye position by specified value
void Camera::updatePosition(glm::vec3 value) {
	eye += value;
}
