#pragma once
class spirit
{
public:
	spirit();
	spirit(vec3 bornPos);
	~spirit() {}
	//	vec3 normal;	
public:
	vec3 pos;
	vec3 speed;
	particle * ParticlesContainer;
	int MaxParticles;
	int lastUsedParticle;
	int generateSpeed;
	GLfloat* g_particule_position_size_data;
	GLubyte* g_particule_color_data;

public:
	void GenerateNewParticles(double time, vec3 skullUp);
	int updateParticles(double deltaTime, vec3 cameraPos);
	void move(double deltaTime, vec3 acc);
private:
	int FindUnusedParticle();
	void SortParticles();
	
};