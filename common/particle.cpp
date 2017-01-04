#include <glm/glm.hpp>
#include <vector>
using namespace glm;
#include "particle.hpp"
particle::particle()
{
	cameradistance = -1;
	life = -1;
} 
void particle::born(vec3 center,vec3 mainDirection, vec3 skullUp)
{
	//	normal = vec3(1,1,1);
	life = 0.95f + float(rand() % 10) / 20; // This particle will live 5 seconds.
	totalLife = 0.95f;
	oldSpeed = 1.0;
	pos = center;
	float spread = 3.0f;
	// Very bad way to generate a random direction; 
	// See for instance http://stackoverflow.com/questions/5408276/python-uniform-spherical-distribution instead,
	// combined with some user-controlled parameters (main direction, spread, etc)
	float horizonAngle = float(rand() % 361) / 360.0 * 2 * 3.1415926;
	float veticalAngle = float(rand() % 181) / 180.0 * 2 * 3.1415926;
	glm::vec3 randomdir = glm::vec3(
		sin(veticalAngle) * cos(horizonAngle),
		cos(veticalAngle),
		sin(veticalAngle) * sin(horizonAngle)
		);
	
	speed = mainDirection + randomdir * (spread + 1.5f * abs(dot(randomdir, skullUp)));
	initSpeed = length(speed);
	color = vec4(0, rand() % 28 + 180, rand() % 28 + 120, (rand() % 256 / 3));
	size = (rand() % 5)* 0.001f + 0.1f;

}

