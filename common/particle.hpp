#pragma once
class particle
{
public:
	particle();
	void born(vec3 center, vec3 mainDirection, vec3 skullUp);
	~particle() {}
public:
	vec3 pos;
	vec3  speed;
	float initSpeed;
	vec4 color;
	float size, angle, weight;
	float life; // Remaining life of the particle. if <0 : dead and unused.
	float totalLife;
	float oldSpeed;
	float cameradistance; // *Squared* distance to the camera. if dead : -1.0f

	bool operator<(const particle& that) const {
		// Sort in reverse order : far particles drawn first.
		return this->cameradistance > that.cameradistance;
	}
};