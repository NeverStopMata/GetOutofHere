#pragma once
class ghost
{
public:
	ghost(vec3 pos,int environment_triangles);
//	float getSpeed();
	~ghost() {}
//	vec3 normal;	
	float size;
//	int type;
//	int hp;
//	int energy;
	float subRotTimes;
	float rotationTimesSum;
	float speed;
	float maxSpeed;
	float SpeedAfterHit;
	float life;
//	bool discovered;
	bool onRoad;
	bool isCrash;
	bool isBack;
	int touchPlane;
	vec3 right;
	vec3 top;
	vec3 rightV;
	vec3 topV;
	vec3 position;
//	vec3 destination;
	bool * mayHit;
	std::vector<vec3> Point_Surface_rposition;
	vec3 lastVelocity;
	vec3 newVelocity;
	vec3 lastTempCam2car;
	
	float SpeedPerFrame;
	void getHurt(double deltaTime, vec3 skullPos);

};