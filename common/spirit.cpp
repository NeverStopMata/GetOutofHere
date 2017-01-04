#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/norm.hpp>
#include <vector>
#include <algorithm>
#include "GL\glew.h"
#include "GLFW\glfw3.h"
using namespace glm;
#include "particle.hpp"
#include "spirit.hpp"
spirit::spirit() {
}
spirit::spirit(vec3 bornPos)
{
	pos = bornPos;
	speed = vec3(0,0,0);
	MaxParticles = 10000;
	ParticlesContainer = new particle[MaxParticles];
	g_particule_position_size_data = new GLfloat[MaxParticles * 4];
	g_particule_color_data = new GLubyte[MaxParticles * 4];
	lastUsedParticle = 0;
	generateSpeed = 20000;

}
int spirit::FindUnusedParticle() {

	for (int i = lastUsedParticle; i<MaxParticles; i++) {
		if (ParticlesContainer[i].life < 0) {
			lastUsedParticle = i;
			return i;
		}
	}

	for (int i = 0; i<lastUsedParticle; i++) {
		if (ParticlesContainer[i].life < 0) {
			lastUsedParticle = i;
			return i;
		}
	}

	return 0; // All particles are taken, override the first one
}

void spirit::SortParticles() {
	std::sort(&ParticlesContainer[0], &ParticlesContainer[MaxParticles]);
}
void spirit::GenerateNewParticles(double time, vec3 skullUp){
	int newparticles = (int)(time*generateSpeed);
	if (newparticles > (int)(0.016f*generateSpeed))
		newparticles = (int)(0.016f*generateSpeed);
	//newparticles = 10000;

	for (int i = 0; i<newparticles; i++) {
		int particleIndex = FindUnusedParticle();
		ParticlesContainer[particleIndex].born(pos, speed, skullUp);
	}
}

int spirit::updateParticles(double deltaTime, vec3 cameraPos) {
	// Simulate all particles
	int ParticlesCount = 0;
	SortParticles();
	for (int i = 0; i<MaxParticles; i++) {

		particle& p = ParticlesContainer[i]; // shortcut

		if (p.life > 0.0f) {

			// Decrease life
			p.life -= p.oldSpeed * deltaTime;
			if (p.life > 0.0f) {

				// Simulate simple physics : gravity only, no collisions
				//p.speed += glm::vec3(0.0f,-9.81f, 0.0f) * (float)delta * 0.5f;
				p.speed = normalize(p.speed) * float(length(p.speed) - p.life / p.totalLife * 0.2);
				p.pos += (vec3(0, 0, 0) + p.speed) * (float)deltaTime;
				p.cameradistance = glm::length2(p.pos - cameraPos);
				//ParticlesContainer[i].pos += glm::vec3(0.0f,10.0f, 0.0f) * (float)delta;

				// Fill the GPU buffer

				g_particule_position_size_data[4 * ParticlesCount + 0] = p.pos.x;
				g_particule_position_size_data[4 * ParticlesCount + 1] = p.pos.y;
				g_particule_position_size_data[4 * ParticlesCount + 2] = p.pos.z;

				g_particule_position_size_data[4 * ParticlesCount + 3] = p.size;

				g_particule_color_data[4 * ParticlesCount + 0] = p.color.x;
				g_particule_color_data[4 * ParticlesCount + 1] = p.color.y * p.life / p.totalLife;
				g_particule_color_data[4 * ParticlesCount + 2] = p.color.z;
				g_particule_color_data[4 * ParticlesCount + 3] = 255 * p.life / p.totalLife;

			}
			else {
				// Particles that just died will be put at the end of the buffer in SortParticles();
				p.cameradistance = -1.0f;
			}

			ParticlesCount++;

		}
	}
	return ParticlesCount;
}
void spirit::move(double deltaTime, vec3 acc) {
	speed = speed + acc * (float)deltaTime;
	float v = length(speed);
	const float windage = 0.2;
	v -= windage * v * v * deltaTime;
	if (v < 0)
		v = 0;
	speed = normalize(speed) * v;
	pos = pos + speed * (float)deltaTime;
}