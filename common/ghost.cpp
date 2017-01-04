#include <glm/glm.hpp>
#include <vector>
using namespace glm;
#include "ghost.h"
ghost::ghost(vec3 pos, int environment_triangles)
{
//	normal = vec3(1,1,1);
	size = 0.042;
//	hp = 100;
//	energy = 0;
	speed = 0;
	maxSpeed = 1.5;
	subRotTimes = 0;
	rotationTimesSum = 10;
//	discovered = false;
	onRoad = false;
	isCrash = false;
	isBack = false;
	touchPlane = -1;
	top = vec3(0, 1, 0);
	right = vec3(1, 0, 0);
	topV = vec3(0, 1, 0);
	rightV = vec3(1, 0, 0);
	position = pos;
//	destination = des;
	mayHit = new bool[environment_triangles];
	SpeedPerFrame = 0;
	life = 100;
	for (int i = 0; i < environment_triangles; i++)
	{ 
		mayHit[i] = false;
		Point_Surface_rposition.push_back(vec3(100.0, 1.5, 1.5));
	}

}
void ghost::getHurt(double deltaTime, vec3 skullPos)
{
	float distance;
	if (distance = (length(skullPos - position)) < 1.0)
		life -= deltaTime * 1.0 / length(skullPos - position);
	else
		life += deltaTime * 0.001;
}