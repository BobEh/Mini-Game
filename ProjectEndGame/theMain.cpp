#include "GLCommon.h"
#include <Windows.h>
#include <glm/glm.hpp>
#include <glm/vec3.hpp> // glm::vec3
#include <glm/vec4.hpp> // glm::vec4
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/gtc/matrix_transform.hpp>
// glm::translate, glm::rotate, glm::scale, glm::perspective
#include <glm/gtc/type_ptr.hpp> // glm::value_ptr

#include <glm/gtx/projection.hpp>

#include <stdlib.h>		// c libs
#include <stdio.h>		// c libs

#include <iostream>		// C++ IO standard stuff
#include <map>			// Map aka "dictonary" 

#include "cModelLoader.h"	
#include "cVAOManager.h"		// NEW
//#include "cGameObject.h"

#include "cShaderManager.h"

#include <sstream>
#include <fstream>

#include <limits.h>
#include <float.h>

// The Physics function
#include "PhysicsStuff.h"
#include "cPhysics.h"

#include "DebugRenderer/cDebugRenderer.h"
#include <pugixml/pugixml.hpp>
#include <pugixml/pugixml.cpp>
#include "cLight.h"
#include "cMediator.h"
#include "cObjectFactory.h"
#include "AABBStuff.h"
#include "LowPassFilter.h"
#include "cFBO.h"

#include "cSimpleAssimpSkinnedMeshLoader_OneMesh.h";

// Used to visualize the attenuation of the lights...
#include "LightManager/cLightHelper.h"


// Adding Textures now
#include "cBasicTextureManager.h"

// Adding fly Camera
#include "cFlyCamera.h"

cBasicTextureManager* g_pTextureManager = NULL;
cFlyCamera* g_pFlyCamera = NULL;

bool g_MouseIsInsideWindow = false;
bool g_MouseLeftButtonIsDown = false;

using namespace pugi;

xml_document document;
std::string gameDataLocation = "gameData.xml";
xml_parse_result result = document.load_file(gameDataLocation.c_str());
std::ofstream file;
xml_node root_node = document.child("GameData");
xml_node lightData = root_node.child("LightData");
xml_node rampData = root_node.child("RampData");
xml_node ballData = root_node.child("BallData");
xml_node ballLightData = root_node.child("BallLightData");

void CalcAABBsForMeshModel(cMesh& theMesh);
void SetUpTextureBindingsForObject(
	cGameObject* pCurrentObject,
	GLint shaderProgID);

extern std::map<unsigned long long /*ID*/, cAABB*> g_mapAABBs_World;

cAABB* pCurrentAABBLeft;
cAABB* pCurrentAABBRight;
cAABB* pCurrentAABBFront;
cAABB* pCurrentAABBBack;

cFBO* pTheFBO = NULL;

glm::vec2 waterOffset;

bool fileChanged = false;
bool displayAABBs = false;

void DrawObject(glm::mat4 m, iObject* pCurrentObject, GLint shaderProgID, cVAOManager* pVAOManager);

//glm::vec3 borderLight3 = glm::vec3(0.0f, -149.0f, 0.0f);
//glm::vec3 borderLight4 = glm::vec3(0.0f, 200.0f, 0.0f);
//glm::vec3 borderLight5 = glm::vec3(0.0f, 0.0f, -199.0f);
//glm::vec3 borderLight6 = glm::vec3(0.0f, 0.0f, 199.0f);

cMediator* pMediator = cMediator::createMediator();

unsigned int currentRamp = 0;

cLight* pMainLight = new cLight();
cLight* pMainLight1 = new cLight();
std::vector<cLight*> pLightsVec;
unsigned int currentLight = 0;
cLight* pCorner1Light = new cLight();
cLight* pCorner2Light = new cLight();
cLight* pCorner3Light = new cLight();
cLight* pCorner4Light = new cLight();

float cameraLeftRight = 0.0f;

glm::vec3 cameraEye = glm::vec3(0.0, 80.0, -280.0);
glm::vec3 cameraTarget = glm::vec3(pMainLight->getPositionX(), pMainLight->getPositionY(), pMainLight->getPositionZ());
glm::vec3 upVector = glm::vec3(0.0f, 1.0f, 0.0f);

float SpotInnerAngle1 = 5.0f;
float cornerLightSpotOuterAngle1 = 7.5f;

//mainLight
// This is a "normalized" direction
// (i.e. the length is 1.0f)

bool bLightDebugSheresOn = false;

bool laserActive = false;

bool jumping = false;
bool rolling = false;
bool attacking = false;

bool onGround = false;
bool onPlatform = false;

glm::mat4 calculateWorldMatrix(iObject* pCurrentObject);

std::string currentAnimationName;
glm::vec3 g_HACK_vec3_BoneLocationFK = glm::vec3(0.0f);

// Load up my "scene"  (now global)
std::vector<iObject*> g_vec_pGameObjects;
std::vector<iObject*> g_vec_pEnvironmentObjects;
std::vector<iObject*> g_vec_pGameFBOObjects;
std::vector<iObject*> g_vec_pFloorObjects;
std::vector<std::string> g_vec_pAnimations;
std::vector<iObject*> g_vec_pExplosionObjects;
std::map<std::string /*FriendlyName*/, iObject*> g_map_GameObjectsByFriendlyName;

void AddAnimation(std::string animation)
{
	for (int i = 0; i < g_vec_pAnimations.size(); i++)
	{
		if (g_vec_pAnimations.at(i) == animation)
		{
			return;
		}
	}
	g_vec_pAnimations.push_back(animation);
}


// returns NULL (0) if we didn't find it.
iObject* pFindObjectByFriendlyName(std::string name);
iObject* pFindObjectByFriendlyNameMap(std::string name);

//bool g_BallCollided = false;

void cursor_enter_callback(GLFWwindow* window, int entered)
{
	if (entered)
	{
		::g_MouseIsInsideWindow = true;
		std::cout << "Mouse moved inside window" << std::endl;
	}
	else
	{
		::g_MouseIsInsideWindow = false;
		std::cout << "Mouse moved outside window" << std::endl;
	}
	return;
}//cursor_enter_callback(...

bool isShiftKeyDownByAlone(int mods)
{
	if (mods == GLFW_MOD_SHIFT)
	{
		// Shift key is down all by itself
		return true;
	}
	return false;
}

bool isCtrlKeyDownByAlone(int mods)
{
	if (mods == GLFW_MOD_CONTROL)
	{
		return true;
	}
	return false;
}

bool isOnlyAltKeyDown(int mods)
{
	if (mods == GLFW_MOD_ALT)
	{
		return true;
	}
	return false;
}

bool isShiftDown(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT)) { return true; }
	if (glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT)) { return true; }
	// both are up
	return false;
}

bool isCtrlDown(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL)) { return true; }
	if (glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL)) { return true; }
	// both are up
	return false;
}

bool isAltDown(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_LEFT_ALT)) { return true; }
	if (glfwGetKey(window, GLFW_KEY_RIGHT_ALT)) { return true; }
	// both are up
	return false;
}

bool areAllModifiersUp(GLFWwindow* window)
{
	if (isShiftDown(window)) { return false; }
	if (isCtrlDown(window)) { return false; }
	if (isAltDown(window)) { return false; }
	// Yup, they are all UP
	return true;
}

// Mouse (cursor) callback
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{

	return;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	// A regular mouse wheel returns the y value
	::g_pFlyCamera->setMouseWheelDelta(yoffset);

	//	std::cout << "Mouse wheel: " << yoffset << std::endl;

	return;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		::g_MouseLeftButtonIsDown = true;
		std::cout << "Left Clicked " << std::endl;
	}
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
	{
		::g_MouseLeftButtonIsDown = false;
		std::cout << "Left Click released " << std::endl;
	}

	return;
}

void ProcessAsyncMouse(GLFWwindow* window)
{
	double x, y;
	glfwGetCursorPos(window, &x, &y);

	::g_pFlyCamera->setMouseXY(x, y);

	const float MOUSE_SENSITIVITY = 0.1f;

	//	std::cout << ::g_pFlyCamera->getMouseX() << ", " << ::g_pFlyCamera->getMouseY() << std::endl;

		// Mouse left (primary?) button pressed? 
		// AND the mouse is inside the window...
	if ((glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
		&& ::g_MouseIsInsideWindow)
	{
		// Mouse button is down so turn the camera
		::g_pFlyCamera->Yaw_LeftRight(-::g_pFlyCamera->getDeltaMouseX() * MOUSE_SENSITIVITY);

		::g_pFlyCamera->Pitch_UpDown(::g_pFlyCamera->getDeltaMouseY() * MOUSE_SENSITIVITY);

	}

	// Adjust the mouse speed
	if (::g_MouseIsInsideWindow)
	{
		const float MOUSE_WHEEL_SENSITIVITY = 0.1f;

		// Adjust the movement speed based on the wheel position
		::g_pFlyCamera->movementSpeed += (::g_pFlyCamera->getMouseWheel() * MOUSE_WHEEL_SENSITIVITY);
		if (::g_pFlyCamera->movementSpeed <= 0.0f)
		{
			::g_pFlyCamera->movementSpeed = 0.0f;
		}
	}


	// HACK 
	::g_pFlyCamera->movementSpeed = 2.0f;

	return;
}//ProcessAsyncMouse(...

void ProcessAsyncKeys(GLFWwindow* window)
{
	const float CAMERA_MOVE_SPEED_SLOW = 0.1f;
	const float CAMERA_MOVE_SPEED_FAST = 1.0f;

	const float CAMERA_TURN_SPEED = 0.1f;
	iObject* pEagle = pFindObjectByFriendlyName("eagle");

	// WASD + q = "up", e = down		y axis = up and down
	//									x axis = left and right
	//									z axis = forward and backward
	// 

	//float cameraSpeed = CAMERA_MOVE_SPEED_SLOW;
	//if ( glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS  )
	//{
	//	cameraSpeed = CAMERA_MOVE_SPEED_FAST;
	//}

	float cameraMoveSpeed = ::g_pFlyCamera->movementSpeed;

	// If no keys are down, move the camera
	if (areAllModifiersUp(window))
	{
		
		// Note: The "== GLFW_PRESS" isn't really needed as it's actually "1" 
		// (so the if() treats the "1" as true...)


		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		{
			//			g_CameraEye.z += cameraSpeed;
			::g_pFlyCamera->MoveForward_Z(+cameraMoveSpeed);
		}
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)	// "backwards"
		{
			//			g_CameraEye.z -= cameraSpeed;
			::g_pFlyCamera->MoveForward_Z(-cameraMoveSpeed);
		}
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)	// "left"
		{
			//			g_CameraEye.x -= cameraSpeed;
			::g_pFlyCamera->MoveLeftRight_X(-cameraMoveSpeed);
		}
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)	// "right"
		{
			//			g_CameraEye.x += cameraSpeed;
			::g_pFlyCamera->MoveLeftRight_X(+cameraMoveSpeed);
		}
		if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)	// "up"
		{
			::g_pFlyCamera->MoveUpDown_Y(-cameraMoveSpeed);
			//			::g_pFlyCamera->Roll_CW_CCW( +cameraSpeed );
		}
		if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)	// "down"
		{
			//			g_CameraEye.y -= cameraSpeed;
			::g_pFlyCamera->MoveUpDown_Y(+cameraMoveSpeed);
			//			::g_pFlyCamera->Roll_CW_CCW( -cameraSpeed );
		}
		//g_pFlyCamera->eye = glm::vec3( pEagle->getPositionXYZ().x, pEagle->getPositionXYZ().y, pEagle->getPositionXYZ().z - 50.0f);
	}//if(AreAllModifiersUp(window)

	// If shift is down, do the rotation camera stuff...
	// If no keys are down, move the camera
	if (isShiftDown(window))
	{
		if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)	// "up"
		{
			::g_pFlyCamera->Roll_CW_CCW(-CAMERA_TURN_SPEED);
			//			::g_pFlyCamera->MoveUpDown_Y( +cameraSpeed );
		}
		if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)	// "down"
		{
			::g_pFlyCamera->Roll_CW_CCW(+CAMERA_TURN_SPEED);
			//			::g_pFlyCamera->MoveUpDown_Y( -cameraSpeed );
		}
	}//IsShiftDown(window)


	return;
}// ProcessAsyncKeys(..

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{

	const float CAMERASPEED = 1.0f;
	const float MOVESPEED = 2.0f;

	iObject* pEagle = pFindObjectByFriendlyName("mainCharacter");

	if (!isShiftKeyDownByAlone(mods) && !isCtrlKeyDownByAlone(mods))
	{

		// Move the camera (A & D for left and right, along the x axis)
		if (key == GLFW_KEY_A)
		{
			pEagle->setRotationXYZ(glm::vec3(0.0f,glm::radians(90.0f), 0.0f));
			pEagle->setVelocity(glm::vec3(pEagle->getVelocity().x, 0.0f, 20.0f));
		}
		if (key == GLFW_KEY_D)
		{
			pEagle->setRotationXYZ(glm::vec3(0.0f, glm::radians(-90.0f), 0.0f));
			pEagle->setVelocity(glm::vec3(pEagle->getVelocity().x, 0.0f, 20.0f));
		}
		if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
		{
			pEagle->setVelocity(glm::vec3(pEagle->getVelocity().x, 65.0f, 0.0f));
			jumping = true;
			currentAnimationName = "Jump";
		}
		if (key == GLFW_KEY_S)
		{
			rolling = true;
			currentAnimationName = "Roll";
		}
		if (key == GLFW_KEY_ENTER)
		{
			attacking = true;
			currentAnimationName = "Attack";
		}
	}

	if (isShiftKeyDownByAlone(mods))
	{
		if (key == GLFW_KEY_9)
		{
			bLightDebugSheresOn = false;
		}
		if (key == GLFW_KEY_0)
		{
			bLightDebugSheresOn = true;
		}
		// switch lights to control
		if (key == GLFW_KEY_M)
		{
			currentLight = 0;		// Move the camera -0.01f units
		}
		// move the light
		if (key == GLFW_KEY_A)
		{
			//pLightsVec.at(0)->_PositionX -= CAMERASPEED;
			pEagle->setRotationXYZ(glm::vec3(0.0f, glm::radians(90.0f), 0.0f));
			pEagle->setVelocity(glm::vec3(0.0f, 0.0f, 40.0f));
		}
		if (key == GLFW_KEY_D)
		{
			//pLightsVec.at(0)->_PositionX += CAMERASPEED;
			pEagle->setRotationXYZ(glm::vec3(0.0f, glm::radians(-90.0f), 0.0f));
			pEagle->setVelocity(glm::vec3(0.0f, 0.0f, 40.0f));
		}

		// Move the camera (Q & E for up and down, along the y axis)
		if (key == GLFW_KEY_Q)
		{
			pLightsVec.at(0)->_PositionY -= CAMERASPEED;
		}
		if (key == GLFW_KEY_E)
		{
			pLightsVec.at(0)->_PositionY += CAMERASPEED;
		}

		// Move the camera (W & S for towards and away, along the z axis)
		if (key == GLFW_KEY_W)
		{
			pLightsVec.at(0)->_PositionZ -= CAMERASPEED;
		}
		if (key == GLFW_KEY_S)
		{
			pLightsVec.at(0)->_PositionZ += CAMERASPEED;
		}

		if (key == GLFW_KEY_K)
		{
			for (int i = 0; i < pLightsVec.size(); i++)
			{
				std::string currentNodeName = pLightsVec.at(i)->getNodeName();
				xml_node LightToChange = lightData.child(currentNodeName.c_str());
				std::vector<std::string> changeData = pLightsVec.at(i)->getAllDataStrings();
				//int index = 0;
				//changeData.push_back(std::to_string(pLightsVec.at(i)->getPositionX()));
				//changeData.push_back(std::to_string(pLightsVec.at(i)->getPositionY()));
				//changeData.push_back(std::to_string(pLightsVec.at(i)->getPositionZ()));
				//changeData.push_back(std::to_string(pLightsVec.at(i)->getConstAtten()));
				//changeData.push_back(std::to_string(pLightsVec.at(i)->getLinearAtten()));
				//changeData.push_back(std::to_string(pLightsVec.at(i)->getQuadraticAtten()));
				//changeData.push_back(std::to_string(pLightsVec.at(i)->getInnerSpot()));
				//changeData.push_back(std::to_string(pLightsVec.at(i)->getOuterSpot()));
				//Set data to xml to set positions

				int index = 0;
				for (xml_node dataNode = LightToChange.child("PositionX"); dataNode; dataNode = dataNode.next_sibling())
				{
					//LightToChange->first_node("PositionX")->value(changeData.at(i).c_str());
					//LightToChange->first_node("PositionY")->value(changeData.at(i).c_str());
					//LightToChange->first_node("PositionZ")->value(changeData.at(i).c_str());
					//LightToChange->first_node("ConstAtten")->value(changeData.at(i).c_str());
					//LightToChange->first_node("LinearAtten")->value(changeData.at(i).c_str());
					//LightToChange->first_node("QuadraticAtten")->value(changeData.at(i).c_str());
					//LightToChange->first_node("SpotInnerAngle")->value(changeData.at(i).c_str());
					//LightToChange->first_node("SpotOuterAngle")->value(changeData.at(i).c_str());

					//std::string changeString = changeData.at(index);
					//std::cout << changeString << std::endl;
					dataNode.last_child().set_value(changeData.at(index).c_str());
					//std::cout << dataNode->value() << std::endl;
					//dataNode = dataNode->next_sibling();
					index++;
				}
				//for (xml_node<>* dataNode = LightToChange->first_node(); dataNode; dataNode = dataNode->next_sibling())
				//{
				//	//assert(index < changeData.size());
				//	const char * stringToChange = changeData.at(index).c_str();
				//	dataNode->value(stringToChange);
				//	file.open(gameDataLocation);
				//	file << "<?xml version='1.0' encoding='utf-8'?>\n";
				//	file << document;
				//	file.close();
				//	index++;
				//}
			}
			fileChanged = true;
		}
		//if (key == GLFW_KEY_V)
		//{
		//	if (pCurrentLight == "mainLight")
		//		mainLightPosition -= 0.1f;
		//}
		//if (key == GLFW_KEY_B)
		//{
		//	if (pCurrentLight == "mainLight")
		//		mainLightPosition += 0.1f;
		//}
		//if (key == GLFW_KEY_N)
		//{
		//	if (pCurrentLight == "mainLight")
		//		mainLightPosition -= 0.1f;
		//}
		//if (key == GLFW_KEY_M)
		//{
		//	if (pCurrentLight == "mainLight")
		//		mainLightPosition += 0.1f;
		//}


		if (key == GLFW_KEY_9)
		{
			bLightDebugSheresOn = false;
		}

	}//if (isShiftKeyDownByAlone(mods))

	if (isCtrlKeyDownByAlone(mods))
	{
		// move the shpere

		if (key == GLFW_KEY_R && action == GLFW_PRESS)
		{
			pEagle->setPositionXYZ(glm::vec3(0.0f, 20.0f, 0.0f));
		}
		
		if (key == GLFW_KEY_D)
		{
			//pSphere->rotationXYZ -= glm::vec3(CAMERASPEED, 0.0f, 0.0f);
/*			if(pEagle->getAccel().x > -6.0f)
				pEagle->setAccel(glm::vec3(pEagle->getAccel().x - MOVESPEED, 0.0f, 0.0f));	*/	// Move the camera -0.01f units
			//pEagle->setRotationXYZ(glm::quat(glm::vec3( pEagle->getRotationXYZ().x, pEagle->getRotationXYZ().y + glm::radians(0.1f), pEagle->getRotationXYZ().z)));

			glm::quat rotation = glm::quat(glm::vec3(0.0f, glm::radians(-1.0f), 0.0f));
			pEagle->setRotationXYZ(pEagle->getRotationXYZ()* rotation);
		}
		if (key == GLFW_KEY_A)
		{
			//pSphere->rotationXYZ += glm::vec3(CAMERASPEED, 0.0f, 0.0f);
/*			if (pEagle->getAccel().x < 6.0f)
				pEagle->setAccel(glm::vec3(pEagle->getAccel().x + MOVESPEED, 0.0f, 0.0f));*/		// Move the camera +0.01f units
			//glm::quat rotation = glm::quat(glm::vec3(0.0f, glm::radians(1.0f), 0.0f));
			//rotation *= 0.03f;
			glm::quat rotation = glm::quat(glm::vec3(0.0f, glm::radians(1.0f), 0.0f));
			pEagle->setRotationXYZ(pEagle->getRotationXYZ() * rotation);
		}

		if (key == GLFW_KEY_M && action == GLFW_PRESS)
		{
			if (pEagle->getIsWireframe())
			{
				pEagle->setIsWireframe(false);
			}
			else
			{
				pEagle->setIsWireframe(true);
			}
		}
		//g_pFlyCamera->eye = glm::vec3(pEagle->getPositionXYZ().x, pEagle->getPositionXYZ().y, pEagle->getPositionXYZ().z - 50.0f);

		// Move the camera (Q & E for up and down, along the y axis)
		if (key == GLFW_KEY_Q)
		{
/*			if (pEagle->getAccel().y > -6.0f)
				pEagle->setAccel(glm::vec3(0.0f, pEagle->getAccel().y - MOVESPEED, 0.0f));*/			// Move the camera -0.01f units
			//pEagle->setRotationXYZ(glm::quat(glm::vec3(pEagle->getRotationXYZ().x + glm::radians(0.1f), pEagle->getRotationXYZ().y, pEagle->getRotationXYZ().z)));

			glm::quat rotation = glm::quat(glm::vec3(glm::radians(-1.0f), 0.0f, 0.0f));
			pEagle->setRotationXYZ(pEagle->getRotationXYZ()* rotation);
		}
		if (key == GLFW_KEY_E)
		{
/*			if (pEagle->getAccel().y < 6.0f)
				pEagle->setAccel(glm::vec3(0.0f, pEagle->getAccel().y + MOVESPEED, 0.0f));*/			// Move the camera +0.01f units
			//pEagle->setRotationXYZ(glm::quat(glm::vec3(pEagle->getRotationXYZ().x - glm::radians(0.1f), pEagle->getRotationXYZ().y, pEagle->getRotationXYZ().z)));

			glm::quat rotation = glm::quat(glm::vec3(glm::radians(1.0f), 0.0f, 0.0f));
			pEagle->setRotationXYZ(pEagle->getRotationXYZ()* rotation);
			
		}
		if (key == GLFW_KEY_Z)
		{
			/*			if (pEagle->getAccel().y > -6.0f)
							pEagle->setAccel(glm::vec3(0.0f, pEagle->getAccel().y - MOVESPEED, 0.0f));*/			// Move the camera -0.01f units
							//pEagle->setRotationXYZ(glm::quat(glm::vec3(pEagle->getRotationXYZ().x + glm::radians(0.1f), pEagle->getRotationXYZ().y, pEagle->getRotationXYZ().z)));

			glm::quat rotation = glm::quat(glm::vec3(0.0f, 0.0f, glm::radians(-1.0f)));
			pEagle->setRotationXYZ(pEagle->getRotationXYZ() * rotation);
		}
		if (key == GLFW_KEY_C)
		{
			/*			if (pEagle->getAccel().y < 6.0f)
							pEagle->setAccel(glm::vec3(0.0f, pEagle->getAccel().y + MOVESPEED, 0.0f));*/			// Move the camera +0.01f units
							//pEagle->setRotationXYZ(glm::quat(glm::vec3(pEagle->getRotationXYZ().x - glm::radians(0.1f), pEagle->getRotationXYZ().y, pEagle->getRotationXYZ().z)));

			glm::quat rotation = glm::quat(glm::vec3(0.0f, 0.0f, glm::radians(1.0f)));
			pEagle->setRotationXYZ(pEagle->getRotationXYZ() * rotation);

		}
		//if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
		//{
		//	if (onGround)
		//	{
		//		pSphere->velocity.y = 10.0f;
		//	}
		//}

		// Move the camera (W & S for towards and away, along the z axis)
		if (key == GLFW_KEY_S)
		{
			//if (pEagle->getAccel().z > -6.0f)
			//	pEagle->setAccel(glm::vec3(0.0f, 0.0f, pEagle->getAccel().z - MOVESPEED));		// Move the camera -0.01f units
			//pEagle->setVelocity(glm::vec3(0.0f, 0.0f, -25.0f));
			currentAnimationName = "Idle";
		}
		if (key == GLFW_KEY_W)
		{
			//if (pEagle->getAccel().z < 6.0f)
			//	pEagle->setAccel(glm::vec3(0.0f, 0.0f, pEagle->getAccel().z + MOVESPEED));		// Move the camera +0.01f units
			//pEagle->setVelocity(glm::vec3(0.0f, 0.0f, 25.0f));
			currentAnimationName = "Walk-Slow";
		}
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		{
			if (pEagle->getAccel().z < 6.0f)
			{
				//pEagle->setAccel(glm::vec3(pEagle->getAccel().x + MOVESPEED, 0.0f, 0.0f));
				pEagle->setAccel(glm::vec3(0.0f, 0.0f, pEagle->getAccel().z + MOVESPEED));
				glm::quat rotation = glm::quat(glm::vec3(0.0f, glm::radians(1.0f), 0.0f));
				pEagle->setRotationXYZ(pEagle->getRotationXYZ()* rotation);
			}
			//else if(pEagle->getAccel().x < 6.0f)
			//	pEagle->setAccel(glm::vec3(pEagle->getAccel().x + MOVESPEED, 0.0f, 0.0f));
			else
			{
				glm::quat rotation = glm::quat(glm::vec3(0.0f, glm::radians(1.0f), 0.0f));
				pEagle->setRotationXYZ(pEagle->getRotationXYZ() * rotation);
			}
		}
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		{
			if (pEagle->getAccel().z < 6.0f)
			{
				//pEagle->setAccel(glm::vec3(pEagle->getAccel().x - MOVESPEED, 0.0f, 0.0f));
				pEagle->setAccel(glm::vec3(0.0f, 0.0f, pEagle->getAccel().z + MOVESPEED));
				glm::quat rotation = glm::quat(glm::vec3(0.0f, glm::radians(-1.0f), 0.0f));
				pEagle->setRotationXYZ(pEagle->getRotationXYZ()* rotation);
			}
			//else if (pEagle->getAccel().x > -6.0f)
			//	pEagle->setAccel(glm::vec3(pEagle->getAccel().x - MOVESPEED, 0.0f, 0.0f));
			else
			{
				glm::quat rotation = glm::quat(glm::vec3(0.0f, glm::radians(-1.0f), 0.0f));
				pEagle->setRotationXYZ(pEagle->getRotationXYZ()* rotation);
			}
		}
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		{
			if (pEagle->getAccel().z > -6.0f)
			{
				//pEagle->setAccel(glm::vec3(pEagle->getAccel().x + MOVESPEED, 0.0f, 0.0f));
				pEagle->setAccel(glm::vec3(0.0f, 0.0f, pEagle->getAccel().z - MOVESPEED));
				glm::quat rotation = glm::quat(glm::vec3(0.0f, glm::radians(1.0f), 0.0f));
				pEagle->setRotationXYZ(pEagle->getRotationXYZ()* rotation);
			}
			//else if (pEagle->getAccel().x < 6.0f)
			//	pEagle->setAccel(glm::vec3(pEagle->getAccel().x + MOVESPEED, 0.0f, 0.0f));
			else
			{
				glm::quat rotation = glm::quat(glm::vec3(0.0f, glm::radians(1.0f), 0.0f));
				pEagle->setRotationXYZ(pEagle->getRotationXYZ()* rotation);
			}
		}
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		{
			if (pEagle->getAccel().z > -6.0f)
			{
				pEagle->setAccel(glm::vec3(0.0f, 0.0f, pEagle->getAccel().z - MOVESPEED));
				//pEagle->setAccel(glm::vec3(pEagle->getAccel().x - MOVESPEED, 0.0f, 0.0f));
				glm::quat rotation = glm::quat(glm::vec3(0.0f, glm::radians(-1.0f), 0.0f));
				pEagle->setRotationXYZ(pEagle->getRotationXYZ()* rotation);
			}
			else
			{
				glm::quat rotation = glm::quat(glm::vec3(0.0f, glm::radians(-1.0f), 0.0f));
				pEagle->setRotationXYZ(pEagle->getRotationXYZ()* rotation);
			}
			//else if (pEagle->getAccel().x > -6.0f)
			//	pEagle->setAccel(glm::vec3(pEagle->getAccel().x - MOVESPEED, 0.0f, 0.0f));
		}

		if (key == GLFW_KEY_LEFT)
		{
			if (pEagle->getAccel().x < 6.0f)
				pEagle->setAccel(glm::vec3(pEagle->getAccel().x + MOVESPEED, 0.0f, 0.0f));
		}

		if (key == GLFW_KEY_RIGHT)
		{
			if (pEagle->getAccel().x > -6.0f)
				pEagle->setAccel(glm::vec3(pEagle->getAccel().x - MOVESPEED, 0.0f, 0.0f));
		}

		if (key == GLFW_KEY_UP)
		{
			if (pEagle->getAccel().y < 6.0f)
				pEagle->setAccel(glm::vec3(0.0f, pEagle->getAccel().y + MOVESPEED, 0.0f));
		}

		if (key == GLFW_KEY_DOWN)
		{
			if (pEagle->getAccel().y > -6.0f)
				pEagle->setAccel(glm::vec3(0.0f, pEagle->getAccel().y - MOVESPEED, 0.0f));
		}

		if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		{
			if (pEagle->getAccel().z < 6.0f)
			{
				//pEagle->setAccel(glm::vec3(pEagle->getAccel().x + MOVESPEED, 0.0f, 0.0f));
				pEagle->setAccel(glm::vec3(0.0f, 0.0f, pEagle->getAccel().z + MOVESPEED));
				glm::quat rotation = glm::quat(glm::vec3(glm::radians(-1.0f), 0.0f, 0.0f));
				pEagle->setRotationXYZ(pEagle->getRotationXYZ()* rotation);
			}
			//else if(pEagle->getAccel().x < 6.0f)
			//	pEagle->setAccel(glm::vec3(pEagle->getAccel().x + MOVESPEED, 0.0f, 0.0f));
			else
			{
				glm::quat rotation = glm::quat(glm::vec3(glm::radians(-1.0f), 0.0f, 0.0f));
				pEagle->setRotationXYZ(pEagle->getRotationXYZ()* rotation);
			}
		}
		if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		{
			if (pEagle->getAccel().z < 6.0f)
			{
				//pEagle->setAccel(glm::vec3(pEagle->getAccel().x - MOVESPEED, 0.0f, 0.0f));
				pEagle->setAccel(glm::vec3(0.0f, 0.0f, pEagle->getAccel().z + MOVESPEED));
				glm::quat rotation = glm::quat(glm::vec3(glm::radians(1.0f), 0.0f, 0.0f));
				pEagle->setRotationXYZ(pEagle->getRotationXYZ()* rotation);
			}
			//else if (pEagle->getAccel().x > -6.0f)
			//	pEagle->setAccel(glm::vec3(pEagle->getAccel().x - MOVESPEED, 0.0f, 0.0f));
			else
			{
				glm::quat rotation = glm::quat(glm::vec3(glm::radians(1.0f), 0.0f, 0.0f));
				pEagle->setRotationXYZ(pEagle->getRotationXYZ()* rotation);
			}
		}
		if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		{
			if (pEagle->getAccel().z > -6.0f)
			{
				//pEagle->setAccel(glm::vec3(pEagle->getAccel().x + MOVESPEED, 0.0f, 0.0f));
				pEagle->setAccel(glm::vec3(0.0f, 0.0f, pEagle->getAccel().z - MOVESPEED));
				glm::quat rotation = glm::quat(glm::vec3(glm::radians(-1.0f), 0.0f, 0.0f));
				pEagle->setRotationXYZ(pEagle->getRotationXYZ()* rotation);
			}
			//else if (pEagle->getAccel().x < 6.0f)
			//	pEagle->setAccel(glm::vec3(pEagle->getAccel().x + MOVESPEED, 0.0f, 0.0f));
			else
			{
				glm::quat rotation = glm::quat(glm::vec3(glm::radians(-1.0f), 0.0f, 0.0f));
				pEagle->setRotationXYZ(pEagle->getRotationXYZ()* rotation);
			}
		}
		if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		{
			if (pEagle->getAccel().z > -6.0f)
			{
				pEagle->setAccel(glm::vec3(0.0f, 0.0f, pEagle->getAccel().z - MOVESPEED));
				//pEagle->setAccel(glm::vec3(pEagle->getAccel().x - MOVESPEED, 0.0f, 0.0f));
				glm::quat rotation = glm::quat(glm::vec3(glm::radians(1.0f), 0.0f, 0.0f));
				pEagle->setRotationXYZ(pEagle->getRotationXYZ()* rotation);
			}
			else
			{
				glm::quat rotation = glm::quat(glm::vec3(glm::radians(1.0f), 0.0f, 0.0f));
				pEagle->setRotationXYZ(pEagle->getRotationXYZ()* rotation);
			}
			//else if (pEagle->getAccel().x > -6.0f)
			//	pEagle->setAccel(glm::vec3(pEagle->getAccel().x - MOVESPEED, 0.0f, 0.0f));
		}

		if (key == GLFW_KEY_1)
		{
			pLightsVec.at(currentLight)->setConstAtten(pLightsVec.at(currentLight)->getConstAtten() * 0.99f);			// 99% of what it was
		}
		if (key == GLFW_KEY_2)
		{
			pLightsVec.at(currentLight)->setConstAtten(pLightsVec.at(currentLight)->getConstAtten() * 1.01f);
		}
		if (key == GLFW_KEY_3)
		{
			pLightsVec.at(currentLight)->setLinearAtten(pLightsVec.at(currentLight)->getLinearAtten() * 0.99f);			// 99% of what it was
		}
		if (key == GLFW_KEY_4)
		{
			pLightsVec.at(currentLight)->setLinearAtten(pLightsVec.at(currentLight)->getLinearAtten() * 1.01f);			// 1% more of what it was
		}
		if (key == GLFW_KEY_5)
		{
			pLightsVec.at(currentLight)->setQuadraticAtten(pLightsVec.at(currentLight)->getQuadraticAtten() * 0.99f);
		}
		if (key == GLFW_KEY_6)
		{
			pLightsVec.at(currentLight)->setQuadraticAtten(pLightsVec.at(currentLight)->getQuadraticAtten() * 1.01f);
		}

		//cGameObject* pShip = pFindObjectByFriendlyName("PirateShip");
		//// Turn the ship around
		//if (key == GLFW_KEY_A)
		//{	// Left
		//	pShip->HACK_AngleAroundYAxis -= 0.01f;
		//	pShip->rotationXYZ.y = pShip->HACK_AngleAroundYAxis;
		//}
		//if (key == GLFW_KEY_D)
		//{	// Right
		//	pShip->HACK_AngleAroundYAxis += 0.01f;
		//	pShip->rotationXYZ.y = pShip->HACK_AngleAroundYAxis;
		//}
		//if (key == GLFW_KEY_W)
		//{	// Faster
		//	pShip->HACK_speed += 0.1f;
		//}
		//if (key == GLFW_KEY_S)
		//{	// Slower
		//	pShip->HACK_speed -= 0.1f;
		//}
	}

	if (isCtrlKeyDownByAlone(mods) && isShiftKeyDownByAlone(mods))
	{

	}


	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);

}

static void error_callback(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}

template <class T>
T randInRange(T min, T max)
{
	double value =
		min + static_cast <double> (rand())
		/ (static_cast <double> (RAND_MAX / (static_cast<double>(max - min))));
	return static_cast<T>(value);
};

void CollideWithFloor(iObject* charachter, iObject* floor);

int main(void)
{
	//std::ifstream gameData(gameDataLocation);


	//std::vector<char> buffer((std::istreambuf_iterator<char>(gameData)), std::istreambuf_iterator<char>());
	//buffer.push_back('\0');

	//document.parse<0>(&buffer[0]);
	//root_node = document.first_node("GameData");
	//lightData = root_node->first_node("LightData");

	//root_node = document.child("GameData");
	//lightData = root_node.child("LightData");

	cModelLoader* pTheModelLoader = new cModelLoader();	// Heap

	cObjectFactory* pFactory = new cObjectFactory();

	GLFWwindow* window;

	glfwSetErrorCallback(error_callback);
	if (!glfwInit())
	{
		exit(EXIT_FAILURE);
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

	window = glfwCreateWindow(640, 480, "Simple example", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwSetKeyCallback(window, key_callback);
	glfwMakeContextCurrent(window);
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
	glfwSwapInterval(1);

	// Mouse callbacks
	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetCursorEnterCallback(window, cursor_enter_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);

	void ProcessAsyncMouse(GLFWwindow * window);
	void ProcessAsyncKeys(GLFWwindow * window);



	cDebugRenderer* pDebugRenderer = new cDebugRenderer();
	pDebugRenderer->initialize();

	//cMesh safePartsMesh;
	//pTheModelLoader->LoadPlyModel("assets/models/Parts_Safe_from_Asteroids_xyz_n.ply", safePartsMesh);

	//cMesh exposedPartsMesh;
	//pTheModelLoader->LoadPlyModel("assets/models/Parts_Exposed_to_Asteroids_xyz_n.ply", exposedPartsMesh);

	std::string assimpErrorString = "";

	//cMesh mainCharacterMesh;
	//if (!pTheModelLoader->LoadModel_Assimp("assets/modelsFBX/RPG-Character(FBX2013).FBX", mainCharacterMesh, assimpErrorString))
	//{
	//	std::cout << "Error: couldn't find the rpg character fbx." << std::endl;
	//}

	//cMesh walkAnimation;
	//if (!pTheModelLoader->LoadModel_Assimp("assets/modelsFBX/RPG-Character_Unarmed-Walk(FBX2013).FBX", walkAnimation, assimpErrorString))
	//{
	//	std::cout << "Error: couldn't find the walk animation fbx." << std::endl;
	//}	

	cMesh enemyMesh;
	pTheModelLoader->LoadPlyModel("assets/models/Android_Robot_All_xyz_n_uv.ply", enemyMesh);

	cMesh piece1Mesh;
	pTheModelLoader->LoadPlyModel("assets/models/Android_Robot_Head.ply", piece1Mesh);

	cMesh piece2Mesh;
	pTheModelLoader->LoadPlyModel("assets/models/Android_Robot_Left_Arm.ply", piece2Mesh);

	cMesh piece3Mesh;
	pTheModelLoader->LoadPlyModel("assets/models/Android_Robot_Left_Leg.ply", piece3Mesh);

	cMesh piece4Mesh;
	pTheModelLoader->LoadPlyModel("assets/models/Android_Robot_Right_Arm.ply", piece4Mesh);

	cMesh piece5Mesh;
	pTheModelLoader->LoadPlyModel("assets/models/Android_Robot_Right_Leg.ply", piece5Mesh);

	cMesh piece6Mesh;
	pTheModelLoader->LoadPlyModel("assets/models/Android_Robot_Torso.ply", piece6Mesh);

	cMesh cubeMesh;
	pTheModelLoader->LoadPlyModel("assets/models/Cube_1_Unit_from_origin_XYZ_uv.ply", cubeMesh);

	cMesh eagleMesh;
	pTheModelLoader->LoadPlyModel("assets/models/Eagle_xyz_n_uv.ply", eagleMesh);

	cMesh sphereMesh;
	pTheModelLoader->LoadPlyModel("assets/models/Sphere_Radius_1_XYZ_n_uv.ply", sphereMesh);


	//CalcAABBsForMeshModel(mountainRangeMesh);

	cShaderManager* pTheShaderManager = new cShaderManager();

	cShaderManager::cShader vertexShad;
	vertexShad.fileName = "assets/shaders/vertexShader01.glsl";

	cShaderManager::cShader fragShader;
	fragShader.fileName = "assets/shaders/fragmentShader01.glsl";

	if (!pTheShaderManager->createProgramFromFile("SimpleShader", vertexShad, fragShader))
	{
		std::cout << "Error: didn't compile the shader" << std::endl;
		std::cout << pTheShaderManager->getLastError();
		return -1;
	}

	GLuint shaderProgID = pTheShaderManager->getIDFromFriendlyName("SimpleShader");


	// Create a VAO Manager...
	// #include "cVAOManager.h"  (at the top of your file)
	cVAOManager* pTheVAOManager = new cVAOManager();
	sModelDrawInfo sphereMeshInfo;
	pTheVAOManager->LoadModelIntoVAO("sphere", 
									 sphereMesh,		// Sphere mesh info
									 sphereMeshInfo,
									 shaderProgID);

	sModelDrawInfo cubeMeshInfo;
	pTheVAOManager->LoadModelIntoVAO("cube",
		cubeMesh,		// Sphere mesh info
		cubeMeshInfo,
		shaderProgID);

	sModelDrawInfo enemyMeshInfo;
	pTheVAOManager->LoadModelIntoVAO("enemy", enemyMesh, enemyMeshInfo, shaderProgID);

	sModelDrawInfo piece1MeshInfo;
	pTheVAOManager->LoadModelIntoVAO("piece1", piece1Mesh, piece1MeshInfo, shaderProgID);

	sModelDrawInfo piece2MeshInfo;
	pTheVAOManager->LoadModelIntoVAO("piece2", piece2Mesh, piece2MeshInfo, shaderProgID);

	sModelDrawInfo piece3MeshInfo;
	pTheVAOManager->LoadModelIntoVAO("piece3", piece3Mesh, piece3MeshInfo, shaderProgID);

	sModelDrawInfo piece4MeshInfo;
	pTheVAOManager->LoadModelIntoVAO("piece4", piece4Mesh, piece4MeshInfo, shaderProgID);

	sModelDrawInfo piece5MeshInfo;
	pTheVAOManager->LoadModelIntoVAO("piece5", piece5Mesh, piece5MeshInfo, shaderProgID);

	sModelDrawInfo piece6MeshInfo;
	pTheVAOManager->LoadModelIntoVAO("piece6", piece6Mesh, piece6MeshInfo, shaderProgID);

	sModelDrawInfo eagleMeshInfo;
	pTheVAOManager->LoadModelIntoVAO("eagle", eagleMesh, eagleMeshInfo, shaderProgID);


	// now texture
	// Texture stuff
	::g_pTextureManager = new cBasicTextureManager();
	::g_pTextureManager->SetBasePath("assets/textures");

	if (!::g_pTextureManager->Create2DTextureFromBMPFile("grassTexture_512.bmp", true))
	{
		std::cout << "Didn't load texture" << std::endl;
	}

	::g_pTextureManager->Create2DTextureFromBMPFile("StoneTex_1024.bmp", true);

	GLint ID = ::g_pTextureManager->getTextureIDFromName("grassTexture_512.bmp");

	::g_pTextureManager->Create2DTextureFromBMPFile("sandTexture_1024.bmp", true);

	::g_pTextureManager->Create2DTextureFromBMPFile("shark.bmp", true);

	::g_pTextureManager->Create2DTextureFromBMPFile("fish.bmp", true);

	::g_pTextureManager->Create2DTextureFromBMPFile("water_800.bmp", true);

	g_pTextureManager->Create2DTextureFromBMPFile("green.bmp", true);

	g_pTextureManager->Create2DTextureFromBMPFile("red.bmp", true);

	//Cube Maps loaded here
	::g_pTextureManager->SetBasePath("assets/textures/cubemaps/");
	std::string errorString;

	//if (::g_pTextureManager->CreateCubeTextureFromBMPFiles("space",
	//													   "TropicalSunnyDayRight2048.bmp", "TropicalSunnyDayLeft2048.bmp",
	//													   "TropicalSunnyDayUp2048.bmp", "TropicalSunnyDayDown2048.bmp",
	//													   "TropicalSunnyDayFront2048.bmp", "TropicalSunnyDayBack2048.bmp", true, errorString ))
	//{

	//first cube map
	/*if (::g_pTextureManager->CreateCubeTextureFromBMPFiles("space",
		"up-the-creek_rt.bmp", "up-the-creek_lf.bmp",
		"up-the-creek_up.bmp", "up-the-creek_dn.bmp",
		"up-the-creek_ft.bmp", "up-the-creek_bk.bmp", true, errorString))
	{
		std::cout << "Space skybox loaded" << std::endl;
	}
	else
	{
		std::cout << "OH NO! " << errorString << std::endl;
	}*/

	//Second cube map
	if (::g_pTextureManager->CreateCubeTextureFromBMPFiles("space",
		"TropicalSunnyDayLeft2048.bmp", "TropicalSunnyDayRight2048.bmp",
		"TropicalSunnyDayDown2048.bmp", "TropicalSunnyDayUp2048.bmp",
		"TropicalSunnyDayFront2048.bmp", "TropicalSunnyDayBack2048.bmp", true, errorString))
	{
		std::cout << "space skybox loaded" << std::endl;
	}
	else
	{
		std::cout << "oh no! " << errorString << std::endl;
	}

	iObject* pMainCharacter = pFactory->CreateObject("sphere");
	cSimpleAssimpSkinnedMesh* theSkinnedMesh = new cSimpleAssimpSkinnedMesh();
	pMainCharacter->setpSm(theSkinnedMesh);
	pMainCharacter->getpSm()->LoadMeshFromFile("mainCharacter", "assets/modelsFBX/RPG-Character(FBX2013).FBX");

	sModelDrawInfo* mainCharacterMeshInfo = pMainCharacter->getpSm()->CreateMeshObjectFromCurrentModel();

	pMainCharacter->getpSm()->LoadMeshAnimation("Run", "assets/modelsFBX/Run.fbx");
	pMainCharacter->getpSm()->LoadMeshAnimation("Walk-Slow", "assets/modelsFBX/RPG-Character_Unarmed-Walk-Slow(FBX2013).FBX");
	pMainCharacter->getpSm()->LoadMeshAnimation("Walk", "assets/modelsFBX/Walking.fbx");
	pMainCharacter->getpSm()->LoadMeshAnimation("Jump", "assets/modelsFBX/Jumping.fbx");
	pMainCharacter->getpSm()->LoadMeshAnimation("Fall", "assets/modelsFBX/RPG-Character_Unarmed-Fall(FBX2013).FBX");
	pMainCharacter->getpSm()->LoadMeshAnimation("Attack", "assets/modelsFBX/RPG-Character_Unarmed-Attack-R3(FBX2013).FBX");
	pMainCharacter->getpSm()->LoadMeshAnimation("Idle", "assets/modelsFBX/RPG-Character_Unarmed-Idle(FBX2013).FBX");
	pMainCharacter->getpSm()->LoadMeshAnimation("Dying", "assets/modelsFBX/RPG-Character_Unarmed-Death1(FBX2013).FBX");
	pMainCharacter->getpSm()->LoadMeshAnimation("Roll", "assets/modelsFBX/Sprinting Forward Roll.fbx");
	currentAnimationName = "Idle";
	pMainCharacter->setMeshName("mainCharacter");
	pMainCharacter->setFriendlyName("mainCharacter");	// We use to search 
	pMainCharacter->setPositionXYZ(glm::vec3(-680.0, 120.0f, 0.0f));
	pMainCharacter->setRotationXYZ(glm::vec3(0.0f, glm::radians(-90.0f), 0.0f));
	pMainCharacter->setScale(0.2f);
	//pMainCharacter->setObjectColourRGBA(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
	pMainCharacter->setDebugColour(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
	pMainCharacter->setVelocity(glm::vec3(0.0f, 0.0f, 0.0f));
	pMainCharacter->setAccel(glm::vec3(0.0f, 0.0f, 0.0f));
	pMainCharacter->setInverseMass(1.0f);
	pMainCharacter->setIsVisible(true);
	pMainCharacter->setIsWireframe(false);
	pMainCharacter->setTexture("StoneTex_1024.bmp", 0);
	pMainCharacter->setTexture("grassTexture_512.bmp", 1);
	pMainCharacter->setTexture("sandTexture_1024.bmp", 2);
	pMainCharacter->setTextureRatio(1, 1);
	pMainCharacter->setTransprancyValue(1.0f);
	::g_vec_pGameObjects.push_back(pMainCharacter);
	if (mainCharacterMeshInfo)
	{
		std::cout << mainCharacterMeshInfo->numberOfVertices << " vertices" << std::endl;
		std::cout << mainCharacterMeshInfo->numberOfTriangles << " triangles" << std::endl;
		std::cout << mainCharacterMeshInfo->numberOfIndices << " indices" << std::endl;

		pTheVAOManager->LoadModelDrawInfoIntoVAO(*mainCharacterMeshInfo, shaderProgID);
	}

	iObject* pEnemy1 = pFactory->CreateObject("sphere");
	pEnemy1->setMeshName("enemy");
	pEnemy1->setFriendlyName("enemy1");	// We use to search 
	pEnemy1->setPositionXYZ(glm::vec3(-30.0, 10.0f, 0.0f));
	pEnemy1->setRotationXYZ(glm::vec3(0.0f, glm::radians(-90.0f), 0.0f));
	pEnemy1->setScale(1.0f);
	//pEnemy1->setObjectColourRGBA(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
	pEnemy1->setDebugColour(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
	pEnemy1->setVelocity(glm::vec3(0.0f, 0.0f, 20.0f));
	pEnemy1->setAccel(glm::vec3(0.0f, 0.0f, 0.0f));
	pEnemy1->setInverseMass(1.0f);
	pEnemy1->setIsVisible(true);
	pEnemy1->setIsWireframe(false);
	pEnemy1->setTexture("green.bmp", 1);
	pEnemy1->setTextureRatio(1, 1);
	pEnemy1->setTransprancyValue(1.0f);
	::g_vec_pGameObjects.push_back(pEnemy1);

	iObject* pEnemy2 = pFactory->CreateObject("sphere");
	pEnemy2->setMeshName("enemy");
	pEnemy2->setFriendlyName("enemy2");	// We use to search 
	pEnemy2->setPositionXYZ(glm::vec3(-60.0, 110.0f, 0.0f));
	pEnemy2->setRotationXYZ(glm::vec3(0.0f, glm::radians(-90.0f), 0.0f));
	pEnemy2->setScale(1.0f);
	//pEnemy2->setObjectColourRGBA(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
	pEnemy2->setDebugColour(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
	pEnemy2->setVelocity(glm::vec3(0.0f, 0.0f, 20.0f));
	pEnemy2->setAccel(glm::vec3(0.0f, 0.0f, 0.0f));
	pEnemy2->setInverseMass(1.0f);
	pEnemy2->setIsVisible(true);
	pEnemy2->setIsWireframe(false);
	pEnemy2->setTexture("green.bmp", 1);
	pEnemy2->setTextureRatio(1, 1);
	pEnemy2->setTransprancyValue(1.0f);
	::g_vec_pGameObjects.push_back(pEnemy2);

	iObject* pEnemy3 = pFactory->CreateObject("sphere");
	pEnemy3->setMeshName("enemy");
	pEnemy3->setFriendlyName("enemy3");	// We use to search 
	pEnemy3->setPositionXYZ(glm::vec3(-540.0, 220.0f, 0.0f));
	pEnemy3->setRotationXYZ(glm::vec3(0.0f, glm::radians(-90.0f), 0.0f));
	pEnemy3->setScale(1.0f);
	//pEnemy3->setObjectColourRGBA(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
	pEnemy3->setDebugColour(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
	pEnemy3->setVelocity(glm::vec3(0.0f, 0.0f, 20.0f));
	pEnemy3->setAccel(glm::vec3(0.0f, 0.0f, 0.0f));
	pEnemy3->setInverseMass(1.0f);
	pEnemy3->setIsVisible(true);
	pEnemy3->setIsWireframe(false);
	pEnemy3->setTexture("green.bmp", 1);
	pEnemy3->setTextureRatio(1, 1);
	pEnemy3->setTransprancyValue(1.0f);
	::g_vec_pGameObjects.push_back(pEnemy3);

	iObject* pEnemy4 = pFactory->CreateObject("sphere");
	pEnemy4->setMeshName("enemy");
	pEnemy4->setFriendlyName("enemy4");	// We use to search 
	pEnemy4->setPositionXYZ(glm::vec3(-540.0, 370.0f, 0.0f));
	pEnemy4->setRotationXYZ(glm::vec3(0.0f, glm::radians(-90.0f), 0.0f));
	pEnemy4->setScale(1.0f);
	//pEnemy4->setObjectColourRGBA(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
	pEnemy4->setDebugColour(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
	pEnemy4->setVelocity(glm::vec3(0.0f, 0.0f, 20.0f));
	pEnemy4->setAccel(glm::vec3(0.0f, 0.0f, 0.0f));
	pEnemy4->setInverseMass(1.0f);
	pEnemy4->setIsVisible(true);
	pEnemy4->setIsWireframe(false);
	pEnemy4->setTexture("green.bmp", 1);
	pEnemy4->setTextureRatio(1, 1);
	pEnemy4->setTransprancyValue(1.0f);
	::g_vec_pGameObjects.push_back(pEnemy4);

	// Sphere and cube
	iObject* pSphere = pFactory->CreateObject("sphere");
	pSphere->setMeshName("sphere");
	pSphere->setFriendlyName("Sphere#1");	// We use to search 
	pSphere->setPositionXYZ(glm::vec3(0.0f, 50.0f, 0.0f));
	pSphere->setRotationXYZ(glm::vec3(0.0f, 0.0f, 0.0f));
	pSphere->setScale(1.0f);
	pSphere->setObjectColourRGBA(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
	//pSphere->setDebugColour(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
	pSphere->setVelocity(glm::vec3(0.0f, 0.0f, 0.0f));
	pSphere->setAccel(glm::vec3(0.0f, 0.0f, 0.0f));
	pSphere->set_SPHERE_radius(1.0f);
	pSphere->setInverseMass(1.0f);
	pSphere->setIsVisible(true);
	pSphere->setIsWireframe(false);
	//::g_vec_pGameObjects.push_back(pSphere);

		// "SkyBox"
	iObject* pSkyBoxSphere = pFactory->CreateObject("mesh");
	pSkyBoxSphere->setMeshName("sphere");
	pSkyBoxSphere->setFriendlyName("skybox");
	pSkyBoxSphere->setPositionXYZ(glm::vec3(0.0f, 0.0f, 0.0f));
	pSkyBoxSphere->setDebugColour(glm::vec4(1.0f, 0.0f, 0.0f,1.0f));
	pSkyBoxSphere->setIsWireframe(false);
	pSkyBoxSphere->setScale(500000.0f);		// 1.0 to 10,000,000
	//pSkyBoxSphere->isWireframe = true;
	//pSkyBoxSphere->debugColour = glm::vec4( 0.0f, 1.0f, 0.0f, 1.0f );
	//pSkyBoxSphere->setTexture("Pizza.bmp",1);
	//pSkyBoxSphere->textureRatio[0] = 1.0f;
	pSkyBoxSphere->setPhysicsShapeType("SPHERE");
	pSkyBoxSphere->setInverseMass(0.0f);


	g_vec_pGameObjects.push_back(pSkyBoxSphere);

	iObject* pFloor = pFactory->CreateObject("mesh");
	pFloor->setMeshName("Floor");
	pFloor->setFriendlyName("Floor");	// We use to search 
	pFloor->setPositionXYZ(glm::vec3(0.0f, 0.0f, 0.0f));
	pFloor->setRotationXYZ(glm::vec3(0.0f, 0.0f, 0.0f));
	pFloor->setScale(1.0f);
	pFloor->setObjectColourRGBA(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
	//pMountainRange->setDebugColour(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
	pFloor->setVelocity(glm::vec3(0.0f, 0.0f, 0.0f));
	pFloor->setAccel(glm::vec3(0.0f, 0.0f, 0.0f));
	pFloor->setInverseMass(0.0f);
	pFloor->setIsVisible(true);
	pFloor->setIsWireframe(false);
	pFloor->setTexture("StoneTex_1024.bmp", 1);
	pFloor->setTextureRatio(1, 1);
	pFloor->setTransprancyValue(1.0f);
	//::g_vec_pEnvironmentObjects.push_back(pFloor);
	
	// Will be moved placed around the scene
	iObject* pDebugSphere = pFactory->CreateObject("sphere");
	pDebugSphere->setMeshName("sphere");
	pDebugSphere->setFriendlyName("debug_sphere");
	pDebugSphere->setPositionXYZ(glm::vec3(0.0f, 0.0f, 0.0f));
	pDebugSphere->setRotationXYZ(glm::vec3(0.0f, 0.0f, 0.0f));
	pDebugSphere->setScale(0.1f);
	//	pDebugSphere->objectColourRGBA = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
	pDebugSphere->setDebugColour(glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
	pDebugSphere->setIsWireframe(true);
	pDebugSphere->setInverseMass(0.0f);			// Sphere won't move
	pDebugSphere->setIsVisible(false);

	g_vec_pGameObjects.push_back(pDebugSphere);

	iObject* pDebugCube = pFactory->CreateObject("sphere");
	pDebugCube->setMeshName("cube");
	pDebugCube->setFriendlyName("debug_cube");
	pDebugCube->setPositionXYZ(glm::vec3(0.0f, 0.0f, 0.0f));
	pDebugCube->setRotationXYZ(glm::vec3(0.0f, 0.0f, 0.0f));
	pDebugCube->setScale(1.0f);
	//	pDebugSphere->objectColourRGBA = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
	pDebugCube->setDebugColour(glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
	pDebugCube->setIsWireframe(true);
	pDebugCube->setInverseMass(0.0f);			// Sphere won't move
	pDebugCube->setIsVisible(false);

	g_vec_pGameFBOObjects.push_back(pDebugCube);
	//float xPos = 0.0f;
	float size = 20.0f;
	for (int i = 0; i < 17; i++)
	{
		iObject* pCube = pFactory->CreateObject("mesh");
		pCube->setMeshName("cube");
		std::string cubeName = "cube" + i;
		pCube->setFriendlyName(cubeName);
		pCube->setPositionXYZ(glm::vec3((i * size) * -1.0f, 0.0f, 0.0f));
		pCube->setRotationXYZ(glm::vec3(0.0f, 0.0f, 0.0f));
		pCube->setScale(1.0f);
		//	pCube->objectColourRGBA = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
		pCube->setDebugColour(glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
		pCube->setIsWireframe(false);
		pCube->setInverseMass(0.0f);			// Sphere won't move
		pCube->setIsVisible(false);
		pCube->setTexture("StoneTex_1024.bmp", 1);
		pCube->setTextureRatio(1, 1);
		pCube->setTransprancyValue(1.0f);
		g_vec_pFloorObjects.push_back(pCube);
	}
	float startPoint = (17 * size) + (5 * size);
	for (int i = 0; i < 23; i++)
	{
		iObject* pCube = pFactory->CreateObject("mesh");
		pCube->setMeshName("cube");
		std::string cubeName = "cube" + i;
		pCube->setFriendlyName(cubeName);
		pCube->setPositionXYZ(glm::vec3((startPoint + (i * size)) * -1.0f, 0.0f, 0.0f));
		pCube->setRotationXYZ(glm::vec3(0.0f, 0.0f, 0.0f));
		pCube->setScale(1.0f);
		//	pCube->objectColourRGBA = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
		pCube->setDebugColour(glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
		pCube->setIsWireframe(false);
		pCube->setInverseMass(0.0f);			// Sphere won't move
		pCube->setIsVisible(false);
		pCube->setTexture("StoneTex_1024.bmp", 1);
		pCube->setTextureRatio(1, 1);
		pCube->setTransprancyValue(1.0f);
		g_vec_pFloorObjects.push_back(pCube);
	}

	for (int i = 1; i < 18; i++)
	{
		iObject* pCube = pFactory->CreateObject("mesh");
		pCube->setMeshName("cube");
		std::string cubeName = "cube" + i;
		pCube->setFriendlyName(cubeName);
		pCube->setPositionXYZ(glm::vec3(0.0f, (i * size), 0.0f));
		pCube->setRotationXYZ(glm::vec3(0.0f, 0.0f, 0.0f));
		pCube->setScale(1.0f);
		//	pCube->objectColourRGBA = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
		pCube->setDebugColour(glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
		pCube->setIsWireframe(false);
		pCube->setInverseMass(0.0f);			// Sphere won't move
		pCube->setIsVisible(false);
		pCube->setTexture("StoneTex_1024.bmp", 1);
		pCube->setTextureRatio(1, 1);
		pCube->setTransprancyValue(1.0f);
		g_vec_pFloorObjects.push_back(pCube);
	}

	for (int i = 25; i < 26; i++)
	{
		iObject* pCube = pFactory->CreateObject("mesh");
		pCube->setMeshName("cube");
		std::string cubeName = "cube" + i;
		pCube->setFriendlyName(cubeName);
		pCube->setPositionXYZ(glm::vec3(0.0f, (i * size), 0.0f));
		pCube->setRotationXYZ(glm::vec3(0.0f, 0.0f, 0.0f));
		pCube->setScale(1.0f);
		//	pCube->objectColourRGBA = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
		pCube->setDebugColour(glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
		pCube->setIsWireframe(false);
		pCube->setInverseMass(0.0f);			// Sphere won't move
		pCube->setIsVisible(false);
		pCube->setTexture("StoneTex_1024.bmp", 1);
		pCube->setTextureRatio(1, 1);
		pCube->setTransprancyValue(1.0f);
		g_vec_pFloorObjects.push_back(pCube);
	}

	for (int i = 6; i < 26; i++)
	{
		iObject* pCube = pFactory->CreateObject("mesh");
		pCube->setMeshName("cube");
		std::string cubeName = "cube" + i;
		pCube->setFriendlyName(cubeName);
		pCube->setPositionXYZ(glm::vec3(-880.0f, (i * size), 0.0f));
		pCube->setRotationXYZ(glm::vec3(0.0f, 0.0f, 0.0f));
		pCube->setScale(1.0f);
		//	pCube->objectColourRGBA = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
		pCube->setDebugColour(glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
		pCube->setIsWireframe(false);
		pCube->setInverseMass(0.0f);			// Sphere won't move
		pCube->setIsVisible(false);
		pCube->setTexture("StoneTex_1024.bmp", 1);
		pCube->setTextureRatio(1, 1);
		pCube->setTransprancyValue(1.0f);
		g_vec_pFloorObjects.push_back(pCube);
	}

	for (int i = 0; i < 22; i++)
	{
		iObject* pCube = pFactory->CreateObject("mesh");
		pCube->setMeshName("cube");
		std::string cubeName = "cube" + i;
		pCube->setFriendlyName(cubeName);
		pCube->setPositionXYZ(glm::vec3((i * size) * -1.0f, 100.0f, 0.0f));
		pCube->setRotationXYZ(glm::vec3(0.0f, 0.0f, 0.0f));
		pCube->setScale(1.0f);
		//	pCube->objectColourRGBA = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
		pCube->setDebugColour(glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
		pCube->setIsWireframe(false);
		pCube->setInverseMass(0.0f);			// Sphere won't move
		pCube->setIsVisible(false);
		pCube->setTexture("StoneTex_1024.bmp", 1);
		pCube->setTextureRatio(1, 1);
		pCube->setTransprancyValue(1.0f);
		g_vec_pFloorObjects.push_back(pCube);
	}

	for (int i = 0; i < 17; i++)
	{
		iObject* pCube = pFactory->CreateObject("mesh");
		pCube->setMeshName("cube");
		std::string cubeName = "cube" + i;
		pCube->setFriendlyName(cubeName);
		pCube->setPositionXYZ(glm::vec3((i * size) * -1.0f, 360.0f, 0.0f));
		pCube->setRotationXYZ(glm::vec3(0.0f, 0.0f, 0.0f));
		pCube->setScale(1.0f);
		//	pCube->objectColourRGBA = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
		pCube->setDebugColour(glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
		pCube->setIsWireframe(false);
		pCube->setInverseMass(0.0f);			// Sphere won't move
		pCube->setIsVisible(false);
		pCube->setTexture("StoneTex_1024.bmp", 1);
		pCube->setTextureRatio(1, 1);
		pCube->setTransprancyValue(1.0f);
		g_vec_pFloorObjects.push_back(pCube);
	}

	for (int i = 7; i < 10; i++)
	{
		iObject* pCube = pFactory->CreateObject("mesh");
		pCube->setMeshName("cube");
		std::string cubeName = "cube" + i;
		pCube->setFriendlyName(cubeName);
		pCube->setPositionXYZ(glm::vec3((i * size) * -1.0f, 180.0f, 0.0f));
		pCube->setRotationXYZ(glm::vec3(0.0f, 0.0f, 0.0f));
		pCube->setScale(1.0f);
		//	pCube->objectColourRGBA = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
		pCube->setDebugColour(glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
		pCube->setIsWireframe(false);
		pCube->setInverseMass(0.0f);			// Sphere won't move
		pCube->setIsVisible(false);
		pCube->setTexture("StoneTex_1024.bmp", 1);
		pCube->setTextureRatio(1, 1);
		pCube->setTransprancyValue(1.0f);
		g_vec_pFloorObjects.push_back(pCube);
	}

	for (int i = 21; i < 22; i++)
	{
		iObject* pCube = pFactory->CreateObject("mesh");
		pCube->setMeshName("cube");
		std::string cubeName = "cube" + i;
		pCube->setFriendlyName(cubeName);
		pCube->setPositionXYZ(glm::vec3((i * size) * -1.0f, 360.0f, 0.0f));
		pCube->setRotationXYZ(glm::vec3(0.0f, 0.0f, 0.0f));
		pCube->setScale(1.0f);
		//	pCube->objectColourRGBA = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
		pCube->setDebugColour(glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
		pCube->setIsWireframe(false);
		pCube->setInverseMass(0.0f);			// Sphere won't move
		pCube->setIsVisible(false);
		pCube->setTexture("StoneTex_1024.bmp", 1);
		pCube->setTextureRatio(1, 1);
		pCube->setTransprancyValue(1.0f);
		g_vec_pFloorObjects.push_back(pCube);
	}

	for (int i = 26; i < 44; i++)
	{
		iObject* pCube = pFactory->CreateObject("mesh");
		pCube->setMeshName("cube");
		std::string cubeName = "cube" + i;
		pCube->setFriendlyName(cubeName);
		pCube->setPositionXYZ(glm::vec3((i * size) * -1.0f, 360.0f, 0.0f));
		pCube->setRotationXYZ(glm::vec3(0.0f, 0.0f, 0.0f));
		pCube->setScale(1.0f);
		//	pCube->objectColourRGBA = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
		pCube->setDebugColour(glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
		pCube->setIsWireframe(false);
		pCube->setInverseMass(0.0f);			// Sphere won't move
		pCube->setIsVisible(false);
		pCube->setTexture("StoneTex_1024.bmp", 1);
		pCube->setTextureRatio(1, 1);
		pCube->setTransprancyValue(1.0f);
		g_vec_pFloorObjects.push_back(pCube);
	}

	for (int i = 21; i < 22; i++)
	{
		iObject* pCube = pFactory->CreateObject("mesh");
		pCube->setMeshName("cube");
		std::string cubeName = "cube" + i;
		pCube->setFriendlyName(cubeName);
		pCube->setPositionXYZ(glm::vec3((i * size) * -1.0f, 210.0f, 0.0f));
		pCube->setRotationXYZ(glm::vec3(0.0f, 0.0f, 0.0f));
		pCube->setScale(1.0f);
		//	pCube->objectColourRGBA = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
		pCube->setDebugColour(glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
		pCube->setIsWireframe(false);
		pCube->setInverseMass(0.0f);			// Sphere won't move
		pCube->setIsVisible(false);
		pCube->setTexture("StoneTex_1024.bmp", 1);
		pCube->setTextureRatio(1, 1);
		pCube->setTransprancyValue(1.0f);
		g_vec_pFloorObjects.push_back(pCube);
	}

	for (int i = 26; i < 44; i++)
	{
		iObject* pCube = pFactory->CreateObject("mesh");
		pCube->setMeshName("cube");
		std::string cubeName = "cube" + i;
		pCube->setFriendlyName(cubeName);
		pCube->setPositionXYZ(glm::vec3((i * size) * -1.0f, 210.0f, 0.0f));
		pCube->setRotationXYZ(glm::vec3(0.0f, 0.0f, 0.0f));
		pCube->setScale(1.0f);
		//	pCube->objectColourRGBA = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
		pCube->setDebugColour(glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
		pCube->setIsWireframe(false);
		pCube->setInverseMass(0.0f);			// Sphere won't move
		pCube->setIsVisible(false);
		pCube->setTexture("StoneTex_1024.bmp", 1);
		pCube->setTextureRatio(1, 1);
		pCube->setTransprancyValue(1.0f);
		g_vec_pFloorObjects.push_back(pCube);
	}

	for (int i = 32; i < 36; i++)
	{
		iObject* pCube = pFactory->CreateObject("mesh");
		pCube->setMeshName("cube");
		std::string cubeName = "cube" + i;
		pCube->setFriendlyName(cubeName);
		pCube->setPositionXYZ(glm::vec3((i * size) * -1.0f, 110.0f, 0.0f));
		pCube->setRotationXYZ(glm::vec3(0.0f, 0.0f, 0.0f));
		pCube->setScale(1.0f);
		//	pCube->objectColourRGBA = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
		pCube->setDebugColour(glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
		pCube->setIsWireframe(false);
		pCube->setInverseMass(0.0f);			// Sphere won't move
		pCube->setIsVisible(false);
		pCube->setTexture("StoneTex_1024.bmp", 1);
		pCube->setTextureRatio(1, 1);
		pCube->setTransprancyValue(1.0f);
		g_vec_pFloorObjects.push_back(pCube);
	}

	//iObject* pDebugCube = pFactory->CreateObject("mesh");
	//pDebugCube->setMeshName("sphere");
	//pDebugCube->setFriendlyName("debug_cube");
	//pDebugCube->setPositionXYZ(glm::vec3(0.0f, 0.0f, 0.0f));
	//pDebugCube->setRotationXYZ(glm::vec3(0.0f, 0.0f, 0.0f));
	//pDebugCube->setScale(10.0f);
	//pDebugCube->setDebugColour(glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
	//pDebugCube->setIsWireframe(true);
	//pDebugCube->setInverseMass(0.0f);			// Sphere won't move
	//pDebugCube->setIsVisible(false);

	::g_pFlyCamera = new cFlyCamera();
	::g_pFlyCamera->eye = glm::vec3(0.0f, 80.0, -80.0);

	glEnable(GL_DEPTH);			// Write to the depth buffer
	glEnable(GL_DEPTH_TEST);	// Test with buffer when drawing



	cPhysics* pPhsyics = new cPhysics();

	cLightHelper* pLightHelper = new cLightHelper();

	//Get data from xml to set positions of main light
	pMainLight->setNodeName("MainLight");
	xml_node mainLightNode = lightData.child("MainLight");
	pMainLight->setPositionX(std::stof(mainLightNode.child("PositionX").child_value()));
	pMainLight->setPositionY(std::stof(mainLightNode.child("PositionY").child_value()));
	pMainLight->setPositionZ(std::stof(mainLightNode.child("PositionZ").child_value()));
	pMainLight->setPositionXYZ(glm::vec3(std::stof(mainLightNode.child("PositionX").child_value()), std::stof(mainLightNode.child("PositionY").child_value()), std::stof(mainLightNode.child("PositionZ").child_value())));
	pMainLight->setConstAtten(std::stof(mainLightNode.child("ConstAtten").child_value()));
	pMainLight->setLinearAtten(std::stof(mainLightNode.child("LinearAtten").child_value()));
	pMainLight->setQuadraticAtten(std::stof(mainLightNode.child("QuadraticAtten").child_value()));
	pMainLight->setInnerSpot(std::stof(mainLightNode.child("SpotInnerAngle").child_value()));
	pMainLight->setOuterSpot(std::stof(mainLightNode.child("SpotOuterAngle").child_value()));

	pMainLight1->setNodeName("MainLight1");
	xml_node mainLight1Node = lightData.child("MainLight1");
	pMainLight1->setPositionX(std::stof(mainLight1Node.child("PositionX").child_value()));
	pMainLight1->setPositionY(std::stof(mainLight1Node.child("PositionY").child_value()));
	pMainLight1->setPositionZ(std::stof(mainLight1Node.child("PositionZ").child_value()));
	pMainLight1->setPositionXYZ(glm::vec3(std::stof(mainLight1Node.child("PositionX").child_value()), std::stof(mainLight1Node.child("PositionY").child_value()), std::stof(mainLight1Node.child("PositionZ").child_value())));
	pMainLight1->setConstAtten(std::stof(mainLight1Node.child("ConstAtten").child_value()));
	pMainLight1->setLinearAtten(std::stof(mainLight1Node.child("LinearAtten").child_value()));
	pMainLight1->setQuadraticAtten(std::stof(mainLight1Node.child("QuadraticAtten").child_value()));
	pMainLight1->setInnerSpot(std::stof(mainLight1Node.child("SpotInnerAngle").child_value()));
	pMainLight1->setOuterSpot(std::stof(mainLight1Node.child("SpotOuterAngle").child_value()));

	//pMainLight1->setNodeName("SharkSpotLight");
	//pMainLight1->setPositionX(30.0f);
	//pMainLight1->setPositionY(50.0f);
	//pMainLight1->setPositionZ(std::stof(mainLight1Node.child("PositionZ").child_value()));
	//pMainLight1->setPositionXYZ(glm::vec3(std::stof(mainLight1Node.child("PositionX").child_value()), std::stof(mainLight1Node.child("PositionY").child_value()), std::stof(mainLight1Node.child("PositionZ").child_value())));
	//pMainLight1->setConstAtten(std::stof(mainLight1Node.child("ConstAtten").child_value()));
	//pMainLight1->setLinearAtten(std::stof(mainLight1Node.child("LinearAtten").child_value()));
	//pMainLight1->setQuadraticAtten(std::stof(mainLight1Node.child("QuadraticAtten").child_value()));
	//pMainLight1->setInnerSpot(std::stof(mainLight1Node.child("SpotInnerAngle").child_value()));
	//pMainLight1->setOuterSpot(std::stof(mainLight1Node.child("SpotOuterAngle").child_value()));

	pLightsVec.push_back(pMainLight);
	pLightsVec.push_back(pMainLight1);

	int setCount = 0;
	float properYPosition = 0.0f;
	float properPlatformYPosition = 0.0f;

	int time = 1;
	int laserTime = 1;

	bool wasHit = false;
	glm::vec3 hitPosition = glm::vec3(0.0f, 0.0f, 0.0f);

	cLowPassFilter avgDeltaTimeThingy;
	double lastTime = glfwGetTime();

	pTheFBO = new cFBO();
	std::string FBOError;
	//pTheFBO->init(1024, 1024, FBOError);
	if (pTheFBO->init(1080, 1080, FBOError))
	{
		std::cout << "Frame buffer is OK" << std::endl;
	}
	else
	{
		std::cout << "FBO Error: " << FBOError << std::endl;
	}

	int jumpCount = 0;
	int rollCount = 0;
	int dyingCount = 0;
	int attackCount = 0;

	int enemyCount1 = 0;
	int enemyCount2 = 0;
	int enemyCount3 = 0;
	int enemyCount4 = 0;

	int enemy1Health = 3;
	int enemy2Health = 3;
	int enemy3Health = 3;
	int enemy4Health = 3;

	bool gotHit = false;

	while (!glfwWindowShouldClose(window))
	{
		//Draw everything to the external frame buffer
		// (I get the frame buffer ID, and use that)
		glBindFramebuffer(GL_FRAMEBUFFER, pTheFBO->ID);

		if (jumping)
		{
			jumpCount++;
			if (jumpCount >= 150)
			{
				jumping = false;
				jumpCount = 0;
				currentAnimationName = "Idle";
			}
		}

		if (rolling)
		{
			rollCount++;
			if (rollCount >= 120)
			{
				rolling = false;
				rollCount = 0;
				currentAnimationName = "Idle";
			}
		}
		
		if (attacking)
		{
			attackCount++;
			if (attackCount >= 120)
			{
				attacking = false;
				attackCount = 0;
				currentAnimationName = "Idle";

				for (int i = 0; i < g_vec_pGameObjects.size(); i++)
				{
					if (g_vec_pGameObjects.at(i)->getFriendlyName() == "mainCharacter")
					{
						continue;
					}
					else
					{
						g_vec_pGameObjects.at(i)->setTexture("green.bmp", 1);
					}
				}
			}
		}

		if (gotHit)
		{
			dyingCount++;
			if (dyingCount >= 200)
			{
				glfwSetWindowShouldClose(window, GLFW_TRUE);
			}
		}

		enemyCount1++;
		enemyCount2++;
		enemyCount3++;
		enemyCount4++;

		if (enemyCount1 > 400)
		{
			if (pEnemy1->getRotationXYZ().y < 0)
			{
				pEnemy1->setRotationXYZ(glm::vec3(0.0f, glm::radians(90.0f), 0.0f));
			}
			else
			{
				pEnemy1->setRotationXYZ(glm::vec3(0.0f, glm::radians(-90.0f), 0.0f));
			}
			enemyCount1 = 0;
		}
		if (enemyCount2 > 500)
		{
			if (pEnemy2->getRotationXYZ().y < 0)
			{
				pEnemy2->setRotationXYZ(glm::vec3(0.0f, glm::radians(90.0f), 0.0f));
			}
			else
			{
				pEnemy2->setRotationXYZ(glm::vec3(0.0f, glm::radians(-90.0f), 0.0f));
			}
			enemyCount2 = 0;
		}
		if (enemyCount3 > 470)
		{
			if (pEnemy3->getRotationXYZ().y < 0)
			{
				pEnemy3->setRotationXYZ(glm::vec3(0.0f, glm::radians(90.0f), 0.0f));
			}
			else
			{
				pEnemy3->setRotationXYZ(glm::vec3(0.0f, glm::radians(-90.0f), 0.0f));
			}
			enemyCount3 = 0;
		}
		if (enemyCount4 > 450)
		{
			if (pEnemy4->getRotationXYZ().y < 0)
			{
				pEnemy4->setRotationXYZ(glm::vec3(0.0f, glm::radians(90.0f), 0.0f));
			}
			else
			{
				pEnemy4->setRotationXYZ(glm::vec3(0.0f, glm::radians(-90.0f), 0.0f));
			}
			enemyCount4 = 0;
		}
		
		//if (pMainCharacter->getVelocity().z < 1.0f)
		//{
		//	currentAnimationName = "Idle";
		//}

		pTheFBO->clearBuffers(true, true);

		// set the passNumber to 0
		GLint passNumber_UniLoc = glGetUniformLocation(shaderProgID, "passNumber");
		glUniform1i(passNumber_UniLoc, 0);

		double currentTime = glfwGetTime();

		// Frame time... (how many seconds since last frame)
		double deltaTime = currentTime - lastTime;
		lastTime = currentTime;

		const double SOME_HUGE_TIME = 0.1;	// 100 ms;
		if (deltaTime > SOME_HUGE_TIME)
		{
			deltaTime = SOME_HUGE_TIME;
		}

		avgDeltaTimeThingy.addValue(deltaTime);

		waterOffset.x += 0.1f * deltaTime;
		waterOffset.y += 0.017f * deltaTime;

		ProcessAsyncKeys(window);
		ProcessAsyncMouse(window);

		glUseProgram(shaderProgID);

		float ratio;
		int width, height;
		glm::mat4 p, v;

		glfwGetFramebufferSize(window, &width, &height);
		ratio = width / (float)height;

		// Projection matrix
		p = glm::perspective(0.6f,		// FOV
			ratio,			// Aspect ratio
			0.1f,			// Near clipping plane
			10000000.0f);		// Far clipping plane

// View matrix
		v = glm::mat4(1.0f);

		glm::vec3 mainLightPosition = glm::vec3(pMainLight->getPositionX(), pMainLight->getPositionY(), pMainLight->getPositionZ());

		/*v = glm::lookAt(cameraEye,
			pEagle->getPositionXYZ(),
			upVector);*/
		//::g_pFlyCamera->eye.x = cameraEye.x;
		g_pFlyCamera->eye = glm::vec3(-460.0f, 250.0f, -960.0f);
		
		v = glm::lookAt(::g_pFlyCamera->eye,
			glm::vec3(g_pFlyCamera->eye.x, g_pFlyCamera->eye.y,0.0f),
			::g_pFlyCamera->getUpVector());

		std::stringstream ssTitle;
		ssTitle
			<< "X: "
			<< g_pFlyCamera->eye.x << ", "
			<< "Y: "
			<< g_pFlyCamera->eye.y << ", "
			<< "Z: "
			<< g_pFlyCamera->eye.z;
		glfwSetWindowTitle(window, ssTitle.str().c_str());

		glViewport(0, 0, width, height);

		// Clear both the colour buffer (what we see) and the 
		//  depth (or z) buffer.
		//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		int lightIndex = 0;
		for (lightIndex; lightIndex < pLightsVec.size(); ++lightIndex)
		{
			std::string positionString = "theLights[" + std::to_string(lightIndex) + "].position";
			std::string diffuseString = "theLights[" + std::to_string(lightIndex) + "].diffuse";
			std::string specularString = "theLights[" + std::to_string(lightIndex) + "].specular";
			std::string attenString = "theLights[" + std::to_string(lightIndex) + "].atten";
			std::string directionString = "theLights[" + std::to_string(lightIndex) + "].direction";
			std::string param1String = "theLights[" + std::to_string(lightIndex) + "].param1";
			std::string param2String = "theLights[" + std::to_string(lightIndex) + "].param2";

			GLint position = glGetUniformLocation(shaderProgID, positionString.c_str());
			GLint diffuse = glGetUniformLocation(shaderProgID, diffuseString.c_str());
			GLint specular = glGetUniformLocation(shaderProgID, specularString.c_str());
			GLint atten = glGetUniformLocation(shaderProgID, attenString.c_str());
			GLint direction = glGetUniformLocation(shaderProgID, directionString.c_str());
			GLint param1 = glGetUniformLocation(shaderProgID, param1String.c_str());
			GLint param2 = glGetUniformLocation(shaderProgID, param2String.c_str());

			glUniform4f(position, pLightsVec.at(lightIndex)->getPositionX(), pLightsVec.at(lightIndex)->getPositionY(), pLightsVec.at(lightIndex)->getPositionZ(), 1.0f);
			glUniform4f(diffuse, 1.0f, 1.0f, 1.0f, 1.0f);	// White
			glUniform4f(specular, 1.0f, 1.0f, 1.0f, 1.0f);	// White
			glUniform4f(atten, pLightsVec.at(lightIndex)->getConstAtten(),  /* constant attenuation */	pLightsVec.at(lightIndex)->getLinearAtten(),  /* Linear */ pLightsVec.at(lightIndex)->getQuadraticAtten(),	/* Quadratic */  1000000.0f);	// Distance cut off

			glUniform4f(param1, 0.0f /*POINT light*/, 0.0f, 0.0f, 1.0f);
			glUniform4f(param2, 1.0f /*Light is on*/, 0.0f, 0.0f, 1.0f);
		}

		GLint eyeLocation_UL = glGetUniformLocation(shaderProgID, "eyeLocation");

		/*glUniform4f(eyeLocation_UL,
			cameraEye.x, cameraEye.y, cameraEye.z, 1.0f);*/

		glUniform4f(eyeLocation_UL,
			::g_pFlyCamera->eye.x,
			::g_pFlyCamera->eye.y,
			::g_pFlyCamera->eye.z, 1.0f);

		//std::vector<cAABB*> currentAABBVec;


		GLint matView_UL = glGetUniformLocation(shaderProgID, "matView");
		GLint matProj_UL = glGetUniformLocation(shaderProgID, "matProj");

		glUniformMatrix4fv(matView_UL, 1, GL_FALSE, glm::value_ptr(v));
		glUniformMatrix4fv(matProj_UL, 1, GL_FALSE, glm::value_ptr(p));


		// **************************************************
		// **************************************************
		// Loop to draw everything in the scene

		for (int index = 0; index != ::g_vec_pGameObjects.size(); index++)
		{
			glm::mat4 matModel = glm::mat4(1.0f);

			iObject* pCurrentObject = ::g_vec_pGameObjects[index];

			DrawObject(matModel, pCurrentObject,
				shaderProgID, pTheVAOManager);

		}//for (int index...

		for (int index = 0; index != ::g_vec_pEnvironmentObjects.size(); index++)
		{
			glm::mat4 matModel = glm::mat4(1.0f);

			iObject* pCurrentObject = ::g_vec_pEnvironmentObjects[index];

			DrawObject(matModel, pCurrentObject,
				shaderProgID, pTheVAOManager);

		}//for (int index...

		if (currentAnimationName != "Jump" && currentAnimationName != "Roll" && currentAnimationName != "Dying" && currentAnimationName != "Attack")
		{
			if (pMainCharacter->getVelocity().z > 21.0f)
			{
				currentAnimationName = "Run";
			}
			else if (pMainCharacter->getVelocity().z > 5.0f)
			{
				currentAnimationName = "Walk";
			}
			//else if (pMainCharacter->getVelocity().y > 15.0f)
			//{
			//	currentAnimationName = "Jump";
			//}
			else if (pMainCharacter->getVelocity().y < -3.0f || pMainCharacter->getVelocity().y > 3.0f)
			{
				currentAnimationName = "Fall";
			}
			else
			{
				currentAnimationName = "Idle";
			}
		}

		pPhsyics->IntegrationStep(g_vec_pGameObjects, 0.03f);
		pPhsyics->IntegrationStep(g_vec_pExplosionObjects, 0.03f);

		// collisions for floors
		if (pMainCharacter->getVelocity().y < 1.0f)
		{
			for (int i = 0; i < g_vec_pFloorObjects.size(); i++)
			{
				float collisionDistance = glm::distance(pMainCharacter->getPositionXYZ(), g_vec_pFloorObjects.at(i)->getPositionXYZ());

				if (collisionDistance < 10.0f)
				{
					CollideWithFloor(pMainCharacter, g_vec_pFloorObjects.at(i));
				}
			}
		}

		//robot collisions
		for (int i = 0; i < g_vec_pGameObjects.size(); i++)
		{
			if (g_vec_pGameObjects.at(i)->getFriendlyName() == "mainCharacter")
			{
				continue;
			}
			float collisionDistance = glm::distance(pMainCharacter->getPositionXYZ(), g_vec_pGameObjects.at(i)->getPositionXYZ());

			if (collisionDistance < 20.0f)
			{
				if (g_vec_pExplosionObjects.size() > 0)
				{
					break;
				}
				for (int j = 0; j < g_vec_pGameObjects.size(); j++)
				{
					if (g_vec_pGameObjects.at(j)->getFriendlyName() == "mainCharacter")
					{
						continue;
					}
					else
					{
						if (currentAnimationName != "Attack")
						{
							g_vec_pGameObjects.at(j)->setInverseMass(0.0f);
							currentAnimationName = "Dying";
							gotHit = true;
						}
						//if (g_vec_pGameObjects.at(j)->getFriendlyName() == "enemy1")
						//{
						//	enemy1Health--;
						//	g_vec_pGameObjects.at(j)->setTexture("red.bmp", 1);
						//}
						//if (g_vec_pGameObjects.at(j)->getFriendlyName() == "enemy2")
						//{
						//	enemy2Health--;
						//	g_vec_pGameObjects.at(j)->setTexture("red.bmp", 1);
						//}
						//if (g_vec_pGameObjects.at(j)->getFriendlyName() == "enemy3")
						//{
						//	enemy3Health--;
						//	g_vec_pGameObjects.at(j)->setTexture("red.bmp", 1);
						//}
						//if (g_vec_pGameObjects.at(j)->getFriendlyName() == "enemy4")
						//{
						//	enemy4Health--;
						//	g_vec_pGameObjects.at(j)->setTexture("red.bmp", 1);
						//}
						else
						{
							glm::vec3 pointOfImpact = g_vec_pGameObjects.at(i)->getPositionXYZ();

							iObject* pPiece1 = pFactory->CreateObject("sphere");
							pPiece1->setMeshName("piece1");
							pPiece1->setFriendlyName("piece1");	// We use to search 
							pPiece1->setPositionXYZ(pointOfImpact);
							pPiece1->setRotationXYZ(glm::vec3(0.0f, 0.0f, 0.0f));
							pPiece1->setScale(1.0f);
							pPiece1->setVelocity(glm::vec3(randInRange(10.0f, 30.0f), randInRange(10.0f, 30.0f), randInRange(10.0f, 30.0f)));
							pPiece1->setInverseMass(1.0f);
							pPiece1->setIsVisible(true);
							pPiece1->setIsWireframe(false);
							pPiece1->setTexture("green.bmp", 1);
							pPiece1->setTextureRatio(1, 1);
							pPiece1->setTransprancyValue(1.0f);
							::g_vec_pExplosionObjects.push_back(pPiece1);

							iObject* pPiece2 = pFactory->CreateObject("sphere");
							pPiece2->setMeshName("piece2");
							pPiece2->setFriendlyName("piece1");	// We use to search 
							pPiece2->setPositionXYZ(pointOfImpact);
							pPiece2->setRotationXYZ(glm::vec3(0.0f, 0.0f, 0.0f));
							pPiece2->setScale(1.0f);
							pPiece2->setVelocity(glm::vec3(randInRange(10.0f, 30.0f), randInRange(10.0f, 30.0f), randInRange(10.0f, 30.0f)));
							pPiece2->setInverseMass(1.0f);
							pPiece2->setIsVisible(true);
							pPiece2->setIsWireframe(false);
							pPiece2->setTexture("green.bmp", 1);
							pPiece2->setTextureRatio(1, 1);
							pPiece2->setTransprancyValue(1.0f);
							::g_vec_pExplosionObjects.push_back(pPiece2);

							iObject* pPiece3 = pFactory->CreateObject("sphere");
							pPiece3->setMeshName("piece3");
							pPiece3->setFriendlyName("piece1");	// We use to search 
							pPiece3->setPositionXYZ(pointOfImpact);
							pPiece3->setRotationXYZ(glm::vec3(0.0f, 0.0f, 0.0f));
							pPiece3->setScale(1.0f);
							pPiece3->setVelocity(glm::vec3(randInRange(10.0f, 30.0f), randInRange(10.0f, 30.0f), randInRange(10.0f, 30.0f)));
							pPiece3->setInverseMass(1.0f);
							pPiece3->setIsVisible(true);
							pPiece3->setIsWireframe(false);
							pPiece3->setTexture("green.bmp", 1);
							pPiece3->setTextureRatio(1, 1);
							pPiece3->setTransprancyValue(1.0f);
							::g_vec_pExplosionObjects.push_back(pPiece3);

							iObject* pPiece4 = pFactory->CreateObject("sphere");
							pPiece4->setMeshName("piece4");
							pPiece4->setFriendlyName("piece1");	// We use to search 
							pPiece4->setPositionXYZ(pointOfImpact);
							pPiece4->setRotationXYZ(glm::vec3(0.0f, 0.0f, 0.0f));
							pPiece4->setScale(1.0f);
							pPiece4->setVelocity(glm::vec3(randInRange(10.0f, 30.0f), randInRange(10.0f, 30.0f), randInRange(10.0f, 30.0f)));
							pPiece4->setInverseMass(1.0f);
							pPiece4->setIsVisible(true);
							pPiece4->setIsWireframe(false);
							pPiece4->setTexture("green.bmp", 1);
							pPiece4->setTextureRatio(1, 1);
							pPiece4->setTransprancyValue(1.0f);
							::g_vec_pExplosionObjects.push_back(pPiece4);

							iObject* pPiece5 = pFactory->CreateObject("sphere");
							pPiece5->setMeshName("piece5");
							pPiece5->setFriendlyName("piece1");	// We use to search 
							pPiece5->setPositionXYZ(pointOfImpact);
							pPiece5->setRotationXYZ(glm::vec3(0.0f, 0.0f, 0.0f));
							pPiece5->setScale(1.0f);
							pPiece5->setVelocity(glm::vec3(randInRange(10.0f, 30.0f), randInRange(10.0f, 30.0f), randInRange(10.0f, 30.0f)));
							pPiece5->setInverseMass(1.0f);
							pPiece5->setIsVisible(true);
							pPiece5->setIsWireframe(false);
							pPiece5->setTexture("green.bmp", 1);
							pPiece5->setTextureRatio(1, 1);
							pPiece5->setTransprancyValue(1.0f);
							::g_vec_pExplosionObjects.push_back(pPiece5);

							iObject* pPiece6 = pFactory->CreateObject("sphere");
							pPiece6->setMeshName("piece6");
							pPiece6->setFriendlyName("piece1");	// We use to search 
							pPiece6->setPositionXYZ(pointOfImpact);
							pPiece6->setRotationXYZ(glm::vec3(0.0f, 0.0f, 0.0f));
							pPiece6->setScale(1.0f);
							pPiece6->setVelocity(glm::vec3(randInRange(10.0f, 30.0f), randInRange(10.0f, 30.0f), randInRange(10.0f, 30.0f)));
							pPiece6->setInverseMass(1.0f);
							pPiece6->setIsVisible(true);
							pPiece6->setIsWireframe(false);
							pPiece6->setTexture("green.bmp", 1);
							pPiece6->setTextureRatio(1, 1);
							pPiece6->setTransprancyValue(1.0f);
							::g_vec_pExplosionObjects.push_back(pPiece6);

							g_vec_pGameObjects.at(i)->setInverseMass(0.0f);
							g_vec_pGameObjects.at(i)->setIsVisible(false);
							g_vec_pGameObjects.at(i)->setPositionXYZ(glm::vec3(0.0f, 0.0f, 0.0f));
							//g_vec_pGameObjects.erase(g_vec_pGameObjects.begin() + j);
							break;
						}
					}
				}
			}
		}
		
		// collisions for walls
		if (pMainCharacter->getPositionXYZ().x > -10.0f)
		{
			pMainCharacter->setVelocity(glm::reflect(pMainCharacter->getVelocity(), glm::vec3(-1.0f, 0.0f, 0.0f)));
			// lose some energy
			glm::vec3 nComponent = glm::proj(pMainCharacter->getVelocity(), glm::vec3(0.0f, 1.0f, 0.0f));
			// rewind
			glm::vec3 setVelocity = pMainCharacter->getVelocity() - nComponent * 0.2f;
			pMainCharacter->setVelocity(setVelocity);
		}

		if (pMainCharacter->getPositionXYZ().x < -870.0f)
		{
			pMainCharacter->setVelocity(glm::reflect(pMainCharacter->getVelocity(), glm::vec3(1.0f, 0.0f, 0.0f)));
			// lose some energy
			glm::vec3 nComponent = glm::proj(pMainCharacter->getVelocity(), glm::vec3(0.0f, 1.0f, 0.0f));
			// rewind
			glm::vec3 setVelocity = pMainCharacter->getVelocity() - nComponent * 0.2f;
			pMainCharacter->setVelocity(setVelocity);
		}

		if (bLightDebugSheresOn)
		{
			{// Draw where the light is at
				for (int i = 0; i < pLightsVec.size(); ++i)
				{
					glm::mat4 matModel = glm::mat4(1.0f);
					pDebugSphere->setPositionXYZ(pLightsVec.at(i)->getPositionXYZ());
					pDebugSphere->setScale(0.5f);
					pDebugSphere->setDebugColour(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
					pDebugSphere->setIsWireframe(true);
					DrawObject(matModel, pDebugSphere,
						shaderProgID, pTheVAOManager);
					pDebugSphere->setIsVisible(true);
				}
			}

			// Draw spheres to represent the attenuation...
			{   // Draw a sphere at 1% brightness
				glm::mat4 matModel = glm::mat4(1.0f);
				pDebugSphere->setPositionXYZ(pLightsVec.at(currentLight)->getPositionXYZ());
				float sphereSize = pLightHelper->calcApproxDistFromAtten(
					0.01f,		// 1% brightness (essentially black)
					0.001f,		// Within 0.1%  
					100000.0f,	// Will quit when it's at this distance
					pLightsVec.at(currentLight)->getConstAtten(),
					pLightsVec.at(currentLight)->getLinearAtten(),
					pLightsVec.at(currentLight)->getQuadraticAtten());
				pDebugSphere->setScale(sphereSize);
				pDebugSphere->setDebugColour(glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
				pDebugSphere->setIsWireframe(true);
				DrawObject(matModel, pDebugSphere,
					shaderProgID, pTheVAOManager);
			}
			{   // Draw a sphere at 25% brightness
				glm::mat4 matModel = glm::mat4(1.0f);
				pDebugSphere->setPositionXYZ(pLightsVec.at(currentLight)->getPositionXYZ());
				float sphereSize = pLightHelper->calcApproxDistFromAtten(
					0.25f,		// 1% brightness (essentially black)
					0.001f,		// Within 0.1%  
					100000.0f,	// Will quit when it's at this distance
					pLightsVec.at(currentLight)->getConstAtten(),
					pLightsVec.at(currentLight)->getLinearAtten(),
					pLightsVec.at(currentLight)->getQuadraticAtten());
				pDebugSphere->setScale(sphereSize);
				pDebugSphere->setDebugColour(glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
				pDebugSphere->setIsWireframe(true);
				DrawObject(matModel, pDebugSphere,
					shaderProgID, pTheVAOManager);
			}
			{   // Draw a sphere at 50% brightness
				glm::mat4 matModel = glm::mat4(1.0f);
				pDebugSphere->setPositionXYZ(pLightsVec.at(currentLight)->getPositionXYZ());
				float sphereSize = pLightHelper->calcApproxDistFromAtten(
					0.50f,		// 1% brightness (essentially black)
					0.001f,		// Within 0.1%  
					100000.0f,	// Will quit when it's at this distance
					pLightsVec.at(currentLight)->getConstAtten(),
					pLightsVec.at(currentLight)->getLinearAtten(),
					pLightsVec.at(currentLight)->getQuadraticAtten());
				pDebugSphere->setScale(sphereSize);
				pDebugSphere->setDebugColour(glm::vec4(0.0f, 1.0f, 1.0f, 1.0f));
				pDebugSphere->setIsWireframe(true);
				DrawObject(matModel, pDebugSphere,
					shaderProgID, pTheVAOManager);
			}
			{   // Draw a sphere at 75% brightness
				glm::mat4 matModel = glm::mat4(1.0f);
				pDebugSphere->setPositionXYZ(pLightsVec.at(currentLight)->getPositionXYZ());
				float sphereSize = pLightHelper->calcApproxDistFromAtten(
					0.75f,		// 1% brightness (essentially black)
					0.001f,		// Within 0.1%  
					100000.0f,	// Will quit when it's at this distance
					pLightsVec.at(currentLight)->getConstAtten(),
					pLightsVec.at(currentLight)->getLinearAtten(),
					pLightsVec.at(currentLight)->getQuadraticAtten());
				pDebugSphere->setScale(sphereSize);
				pDebugSphere->setDebugColour(glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));
				pDebugSphere->setIsWireframe(true);
				DrawObject(matModel, pDebugSphere,
					shaderProgID, pTheVAOManager);
			}
			{   // Draw a sphere at 95% brightness
				glm::mat4 matModel = glm::mat4(1.0f);
				pDebugSphere->setPositionXYZ(pLightsVec.at(currentLight)->getPositionXYZ());
				float sphereSize = pLightHelper->calcApproxDistFromAtten(
					0.95f,		// 1% brightness (essentially black)
					0.001f,		// Within 0.1%  
					100000.0f,	// Will quit when it's at this distance
					pLightsVec.at(currentLight)->getConstAtten(),
					pLightsVec.at(currentLight)->getLinearAtten(),
					pLightsVec.at(currentLight)->getQuadraticAtten());
				pDebugSphere->setScale(sphereSize);
				pDebugSphere->setDebugColour(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
				pDebugSphere->setIsWireframe(true);
				DrawObject(matModel, pDebugSphere,
					shaderProgID, pTheVAOManager);
			}
		}// if (bLightDebugSheresOn) 

		 // **************************************************
		// *************************************************
		if (fileChanged)
		{
			//file.open(gameDataLocation);
			file << "<?xml version='1.0' encoding='utf-8'?>\n";
			document.save_file(gameDataLocation.c_str());
			//file.close();
		}

		//pDebugRenderer->RenderDebugObjects(v, p, 0.03f);

		// The whole scene is now drawn (to the FBO)

		// 1. Disable the FBO
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// 2. Clear the ACTUAL screen buffer
		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// 3. Use the FBO colour texture as the texture on that quad.
		// set the passNumber to 1
		glUniform1i(passNumber_UniLoc, 1);

		glActiveTexture(GL_TEXTURE0 + 40);
		glBindTexture(GL_TEXTURE_2D, pTheFBO->colourTexture_0_ID);

		GLuint texSampFBO_UL = glGetUniformLocation(shaderProgID, "secondPassColourTexture");
		glUniform1i(texSampFBO_UL, 40);

		// 4. Draw a single object (a triangle or quad)
		iObject* pQuadOrIsIt = pFindObjectByFriendlyName("debug_cube");
		pQuadOrIsIt->setScale(30.0f);
		//glm::vec3 oldLocation = glm::vec3(::g_pFlyCamera->eye.x, ::g_pFlyCamera->eye.y, ::g_pFlyCamera->eye.z);
		//pQuadOrIsIt->setPositionXYZ(glm::vec3( ::g_pFlyCamera->getAtInWorldSpace().x, ::g_pFlyCamera->getAtInWorldSpace().y, ::g_pFlyCamera->getAtInWorldSpace().z + 300));
		//pQuadOrIsIt->setPositionXYZ(glm::vec3(::g_pFlyCamera->eye.x, ::g_pFlyCamera->eye.y, ::g_pFlyCamera->eye.z + 100));
		pQuadOrIsIt->setPositionXYZ(glm::vec3(pMainCharacter->getPositionXYZ().x, 400.0f, pMainCharacter->getPositionXYZ().x));
		pQuadOrIsIt->setIsWireframe(false);

		// Set the actual screen size
		GLint screenWidth_UnitLoc = glGetUniformLocation(shaderProgID, "screenWidth");
		GLint screenHeight_UnitLoc = glGetUniformLocation(shaderProgID, "screenHeight");

		// Get the "screen" framebuffer size 
		glfwGetFramebufferSize(window, &width, &height);

		glUniform1f(screenWidth_UnitLoc, width); 
		glUniform1f(screenHeight_UnitLoc, height);
		
		// Move the camera
		// Maybe set it to orthographic, etc.
		glm::mat4 matQuad = glm::mat4(1.0f);
		//DrawObject(matQuad, pQuadOrIsIt, shaderProgID, pTheVAOManager);
		
		// set pass number back to 0 to render the rest of the scene
		glUniform1i(passNumber_UniLoc, 0);

		for (int index = 0; index != ::g_vec_pGameObjects.size(); index++)
		{
			glm::mat4 matModel = glm::mat4(1.0f);

			iObject* pCurrentObject = ::g_vec_pGameObjects[index];

			//iObject* testing = pFindObjectByFriendlyName("debug_sphere");
			//testing->setIsVisible(true);
			//testing->setIsWireframe(true);
			//testing->setPositionXYZ(pCurrentObject->getPositionXYZ());
			//testing->setScale(50.0f);

			//DrawObject(matModel, testing, shaderProgID, pTheVAOManager);

			DrawObject(matModel, pCurrentObject,
				shaderProgID, pTheVAOManager);

		}//for (int index...

		//{
		//	pDebugSphere->setIsVisible(true);
		//	glm::mat4 matIdentity(1.0f);

		//	pDebugSphere->setPositionXYZ(g_HACK_vec3_BoneLocationFK);

		//	DrawObject(matIdentity, pDebugSphere, shaderProgID, pTheVAOManager);

		//	pDebugSphere->setIsVisible(false);
		//}

		for (int index = 0; index != ::g_vec_pEnvironmentObjects.size(); index++)
		{
			glm::mat4 matModel = glm::mat4(1.0f);

			iObject* pCurrentObject = ::g_vec_pEnvironmentObjects[index];

			DrawObject(matModel, pCurrentObject,
				shaderProgID, pTheVAOManager);

		}//for (int index...

		for (int index = 0; index != ::g_vec_pFloorObjects.size(); index++)
		{
			glm::mat4 matModel = glm::mat4(1.0f);

			iObject* pCurrentObject = ::g_vec_pFloorObjects[index];

			DrawObject(matModel, pCurrentObject,
				shaderProgID, pTheVAOManager);

		}//for (int index...

		for (int index = 0; index != ::g_vec_pExplosionObjects.size(); index++)
		{
			glm::mat4 matModel = glm::mat4(1.0f);

			iObject* pCurrentObject = ::g_vec_pExplosionObjects[index];

			DrawObject(matModel, pCurrentObject,
				shaderProgID, pTheVAOManager);

			if (pCurrentObject->getVelocity().x < 1.0f && pCurrentObject->getVelocity().y < 1.0f && pCurrentObject->getVelocity().z < 1.0f)
			{
				g_vec_pExplosionObjects.erase(g_vec_pExplosionObjects.begin() + index);
				break;
			}

		}//for (int index...
		
		glm::mat4 skyMatModel2 = glm::mat4(1.0f);

		DrawObject(skyMatModel2, pSkyBoxSphere, shaderProgID, pTheVAOManager);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}// main loop

	glfwDestroyWindow(window);
	glfwTerminate();

	// Delete everything
	delete pTheModelLoader;
	//	delete pTheVAOManager;

		// Watch out!!
		// sVertex* pVertices = new sVertex[numberOfVertsOnGPU];
	//	delete [] pVertices;		// If it's an array, also use [] bracket

	exit(EXIT_SUCCESS);
}

void SetUpTextureBindingsForObject(
	iObject* pCurrentObject,
	GLint shaderProgID)
{

	//// Tie the texture to the texture unit
	//GLuint texSamp0_UL = ::g_pTextureManager->getTextureIDFromName("Pizza.bmp");
	//glActiveTexture(GL_TEXTURE0);				// Texture Unit 0
	//glBindTexture(GL_TEXTURE_2D, texSamp0_UL);	// Texture now assoc with texture unit 0

	// Tie the texture to the texture unit
	GLuint texSamp0_UL = ::g_pTextureManager->getTextureIDFromName(pCurrentObject->getTextures(0));
	glActiveTexture(GL_TEXTURE0);				// Texture Unit 0
	glBindTexture(GL_TEXTURE_2D, texSamp0_UL);	// Texture now assoc with texture unit 0

	GLuint texSamp1_UL = ::g_pTextureManager->getTextureIDFromName(pCurrentObject->getTextures(1));
	glActiveTexture(GL_TEXTURE1);				// Texture Unit 1
	glBindTexture(GL_TEXTURE_2D, texSamp1_UL);	// Texture now assoc with texture unit 0

	GLuint texSamp2_UL = ::g_pTextureManager->getTextureIDFromName(pCurrentObject->getTextures(2));
	glActiveTexture(GL_TEXTURE2);				// Texture Unit 2
	glBindTexture(GL_TEXTURE_2D, texSamp2_UL);	// Texture now assoc with texture unit 0

	GLuint texSamp3_UL = ::g_pTextureManager->getTextureIDFromName(pCurrentObject->getTextures(3));
	glActiveTexture(GL_TEXTURE3);				// Texture Unit 3
	glBindTexture(GL_TEXTURE_2D, texSamp3_UL);	// Texture now assoc with texture unit 0

	// Tie the texture units to the samplers in the shader
	GLint textSamp00_UL = glGetUniformLocation(shaderProgID, "textSamp00");
	glUniform1i(textSamp00_UL, 0);	// Texture unit 0

	GLint textSamp01_UL = glGetUniformLocation(shaderProgID, "textSamp01");
	glUniform1i(textSamp01_UL, 1);	// Texture unit 1

	GLint textSamp02_UL = glGetUniformLocation(shaderProgID, "textSamp02");
	glUniform1i(textSamp02_UL, 2);	// Texture unit 2

	GLint textSamp03_UL = glGetUniformLocation(shaderProgID, "textSamp03");
	glUniform1i(textSamp03_UL, 3);	// Texture unit 3


	GLint tex0_ratio_UL = glGetUniformLocation(shaderProgID, "tex_0_3_ratio");
	glUniform4f(tex0_ratio_UL,
		pCurrentObject->getTextureRatio(0),		// 1.0
		pCurrentObject->getTextureRatio(1),
		pCurrentObject->getTextureRatio(2),
		pCurrentObject->getTextureRatio(3));

	{
		//textureWhatTheWhat
		GLuint texSampWHAT_ID = ::g_pTextureManager->getTextureIDFromName("WhatTheWhat.bmp");
		glActiveTexture(GL_TEXTURE13);				// Texture Unit 13
		glBindTexture(GL_TEXTURE_2D, texSampWHAT_ID);	// Texture now assoc with texture unit 0

		GLint textureWhatTheWhat_UL = glGetUniformLocation(shaderProgID, "textureWhatTheWhat");
		glUniform1i(textureWhatTheWhat_UL, 13);	// Texture unit 13
	}



	return;
}

void CollideWithFloor(iObject* character, iObject* floor)
{
	// TODO:
	// 
	// From our textbook, REAL-TIME COLLISION DETECTION, ERICSON
	// Use intersect_moving_sphere_plane(...inputs...outputs...)
	// to determine if:
	// case A: The sphere did not collide during the timestep.
	// case B: The sphere was already colliding at the beginning of the timestep.
	// case C: The sphere collided during the timestep.
	//
	// case A: Return false to indicate no collision happened.
	//
	// case B: Do the timestep again for this sphere after applying an
	//         impulse that should separate it from the plane.
	// 
	// 1) From our textbook, use closest_point_on_plane(..inputs..) to determine the 
	//    penetration-depth of the sphere at the beginning of the timestep.
	//    (This penetration-depth is the distance the sphere must travel during
	//    the timestep in order to escape the plane.)
	// 2) Use the sphere's center and the closest point on the plane to define
	//    the direction of our impulse vector.
	// 3) Use (penetration-depth / DT) to define the magnitude of our impulse vector.
	//    (The impulse vector is now distance/time ...a velocity!)
	// 4) Apply the impulse vector to sphere velocity.
	// 5) Reset the sphere position.
	// 6) Re-do the integration for the timestep.
	// 7) Return true to indicate a collision has happened.
	// 
	// 
	// case C: Rewind to the point of impact, reflect, then replay.
	//
	// 1) Use the outputs from the Ericson function to determine
	//    and set the sphere position to the point of impact.
	// 2) Reflect the sphere's velocity about the plane's normal vector.
	// 3) Apply some energy loss (to the velocity) in the direction
	//    of the plane's normal vector.
	// 4) Re-integrate the sphere with its new velocity over the remaining
	//    portion of the timestep.
	// 5) Return true to indicate a collision has happened.

	//glm::vec3 c = character->getPositionXYZ();
	//float r = 1.0f;
	//glm::vec3 v = glm::vec3(character->getPositionXYZ().x, (character->getPositionXYZ().y - 2.0f), character->getPositionXYZ().z);
	//glm::vec3 n = floor->GetNormal();
	//float d = floor->GetConstant();
	//float t(0.0f);
	//glm::vec3 q;

	//int result = intersect_moving_sphere_plane(c, r, v, n, d, t, q);
	//std::cout << "I hit the ground!!!! :O" << std::endl;
	//if (result == 0)
	//{
	//	// no collision
	//	return false;
	//}
	//if (result == -1)
	//{
		// already colliding at the beginning of the timestep

	//glm::vec3 pointOnPlane = nCollide::closest_point_on_plane(character->mPreviousPosition, planeShape->GetNormal(), planeShape->GetConstant());
	// figure out an impulse that should have the sphere escape the plane
	float distance = glm::distance(character->getPositionXYZ(), floor->getPositionXYZ());
	if (distance < 9.0f)
	{
		float targetDistance = 10.0f;
		glm::vec3 impulse = glm::vec3(0.0f, 1.0f, 0.0f) * (targetDistance - distance) / 0.03f;
		//reset
		character->setPositionXYZ(glm::vec3(character->getPositionXYZ().x, floor->getPositionXYZ().y + 10.0f, character->getPositionXYZ().z));
		//apply impulse
		//character->setVelocity(character->getVelocity() + impulse);

		return;
	}
	////re-do the timestep
	//IntegrateRigidBody(character, mDeltaTime);

	//return true;
	//}
	// it collided

	// reflect
	character->setVelocity(glm::reflect(character->getVelocity(), glm::vec3(0.0f,1.0f,0.0f)));
	// lose some energy
	glm::vec3 nComponent = glm::proj(character->getVelocity(), glm::vec3(0.0f, 1.0f, 0.0f));
	// rewind
	glm::vec3 setVelocity = character->getVelocity() - nComponent * 0.2f;
	character->setVelocity(setVelocity);

	//character->mPosition = (c + v * t);
	//integrate
	//IntegrateRigidBody(character, mDeltaTime * (1.0f - t));
	//finished
	//return true;
}

static glm::vec2 g_OffsetHACK = glm::vec2(0.0f, 0.0f);

float HACK_FrameTime = 0.0f;

void DrawObject(glm::mat4 m, iObject* pCurrentObject, GLint shaderProgID, cVAOManager* pVAOManager)
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Set the texture bindings and samplers
//
//// Tie the texture to the texture unit
//	GLuint grassTexID = ::g_pTextureManager->getTextureIDFromName("grassTexture_512.bmp");
//	glActiveTexture(GL_TEXTURE0);				// Texture Unit 0
//	glBindTexture(GL_TEXTURE_2D, grassTexID);	// Texture now assoc with texture unit 0
//
//	GLuint sandTexID = ::g_pTextureManager->getTextureIDFromName("sandTexture_512.bmp");
//	glActiveTexture(GL_TEXTURE5);				// Texture Unit 5
//	glBindTexture(GL_TEXTURE_2D, sandTexID);	// Texture now assoc with texture unit 0
//
//
//	// Tie the texture units to the samplers in the shader
//	GLint textSamp01_UL = glGetUniformLocation(shaderProgID, "textSamp01");
//	glUniform1i(textSamp01_UL, 0);	// Texture unit 0
//
//	GLint textSamp02_UL = glGetUniformLocation(shaderProgID, "textSamp02");
//	glUniform1i(textSamp02_UL, 5);	// Texture unit 5
	// ************

	//SetUpTextureBindingsForObject(pCurrentObject, shaderProgID);
	GLint bIsSkyBox_UL = glGetUniformLocation(shaderProgID, "bIsSkyBox");
	if (pCurrentObject->getFriendlyName() != "skybox")
	{
		if (pCurrentObject->getFriendlyName() == "island")
		{
			GLint seaFloorBool = glGetUniformLocation(shaderProgID, "bIsIsland");
			glUniform1f(seaFloorBool, true);
			GLint seaFloor = glGetUniformLocation(shaderProgID, "seaFloor");
			glUniform1f(seaFloor, 0.1f);
		}
		else
		{
			GLint seaFloorBool = glGetUniformLocation(shaderProgID, "bIsIsland");
			glUniform1f(seaFloorBool, false);
		}
		if (pCurrentObject->getFriendlyName() == "water")
		{
			GLint waterBool = glGetUniformLocation(shaderProgID, "bIsWater");
			glUniform1f(waterBool, true);
			GLint waterOffsetShader = glGetUniformLocation(shaderProgID, "waterOffset");
			glUniform2f(waterOffsetShader, waterOffset.x, waterOffset.y);

			//uniform bool useHeightMap;	// If true, use heightmap
			GLint useHeightMap_UL = glGetUniformLocation(shaderProgID, "useHeightMap");
			glUniform1f(useHeightMap_UL, (float)GL_TRUE);

			//uniform sampler2D heightMap;
	//		GLuint heightMapID = ::g_pTextureManager->getTextureIDFromName("IslandHeightMap.bmp");
			GLuint heightMapID = ::g_pTextureManager->getTextureIDFromName("water_800.bmp");
			const int TEXTURE_UNIT_40 = 40;
			glActiveTexture(GL_TEXTURE0 + TEXTURE_UNIT_40);				// Texture Unit 18
			glBindTexture(GL_TEXTURE_2D, heightMapID);	// Texture now assoc with texture unit 0

			// Tie the texture units to the samplers in the shader
			GLint heightMap_UL = glGetUniformLocation(shaderProgID, "heightMap");
			glUniform1i(heightMap_UL, TEXTURE_UNIT_40);	// Texture unit 18

			// ANOTHER HACK
			GLint textOffset_UL = glGetUniformLocation(shaderProgID, "textOffset");
			glUniform2f(textOffset_UL, waterOffset.x, waterOffset.y);
		}
		else
		{
			GLint waterBool = glGetUniformLocation(shaderProgID, "bIsWater");
			glUniform1f(waterBool, false);
			GLint useHeightMap_UL = glGetUniformLocation(shaderProgID, "useHeightMap");
			glUniform1f(useHeightMap_UL, (float)GL_FALSE);
		}
		if (pCurrentObject->getFriendlyName() == "seaFloor")
		{
			GLint sand = glGetUniformLocation(shaderProgID, "bIsSand");
			glUniform1f(sand, true);
		}
		else
		{
			GLint sand = glGetUniformLocation(shaderProgID, "bIsSand");
			glUniform1f(sand, false);
		}
		// Is a regular 2D textured object
		SetUpTextureBindingsForObject(pCurrentObject, shaderProgID);
		glUniform1f(bIsSkyBox_UL, (float)GL_FALSE);

		// Don't draw back facing triangles (default)
		glCullFace(GL_BACK);
	}
	else
	{
		GLint useHeightMap_UL = glGetUniformLocation(shaderProgID, "useHeightMap");
		glUniform1f(useHeightMap_UL, (float)GL_FALSE);
		GLint seaFloorBool = glGetUniformLocation(shaderProgID, "bIsIsland");
		glUniform1f(seaFloorBool, false);
		// Draw the back facing triangles. 
		// Because we are inside the object, so it will force a draw on the "back" of the sphere 
		glCullFace(GL_FRONT_AND_BACK);

		glUniform1f(bIsSkyBox_UL, (float)GL_TRUE);

		GLuint skyBoxTextureID = ::g_pTextureManager->getTextureIDFromName("space");
		glActiveTexture(GL_TEXTURE10);				// Texture Unit 26
		glBindTexture(GL_TEXTURE_CUBE_MAP, skyBoxTextureID);	// Texture now assoc with texture unit 0

		// Tie the texture units to the samplers in the shader
		GLint skyBoxSampler_UL = glGetUniformLocation(shaderProgID, "skyBox");
		glUniform1i(skyBoxSampler_UL, 10);	// Texture unit 26
	}

	m = calculateWorldMatrix(pCurrentObject);

	//glUniformMatrix4fv(mvp_location, 1, GL_FALSE, (const GLfloat*) mvp);
	//glUniformMatrix4fv(mvp_location, 1, GL_FALSE, glm::value_ptr(mvp));

	//uniform mat4 matModel;		// Model or World 
	//uniform mat4 matView; 		// View or camera
	//uniform mat4 matProj;
	GLint matModel_UL = glGetUniformLocation(shaderProgID, "matModel");

	glUniformMatrix4fv(matModel_UL, 1, GL_FALSE, glm::value_ptr(m));
	//glUniformMatrix4fv(matView_UL, 1, GL_FALSE, glm::value_ptr(v));
	//glUniformMatrix4fv(matProj_UL, 1, GL_FALSE, glm::value_ptr(p));



	// Find the location of the uniform variable newColour
	GLint newColour_location = glGetUniformLocation(shaderProgID, "newColour");

	glUniform3f(newColour_location,
		pCurrentObject->getObjectColourRGBA().r,
		pCurrentObject->getObjectColourRGBA().g,
		pCurrentObject->getObjectColourRGBA().b);

	GLint diffuseColour_UL = glGetUniformLocation(shaderProgID, "diffuseColour");
	glUniform4f(diffuseColour_UL,
		pCurrentObject->getObjectColourRGBA().r,
		pCurrentObject->getObjectColourRGBA().g,
		pCurrentObject->getObjectColourRGBA().b,
		pCurrentObject->getTransprancyValue());	// 

	GLint specularColour_UL = glGetUniformLocation(shaderProgID, "specularColour");
	glUniform4f(specularColour_UL,
		1.0f,	// R
		1.0f,	// G
		1.0f,	// B
		1000.0f);	// Specular "power" (how shinny the object is)
					// 1.0 to really big (10000.0f)


//uniform vec4 debugColour;
//uniform bool bDoNotLight;
	GLint debugColour_UL = glGetUniformLocation(shaderProgID, "debugColour");
	GLint bDoNotLight_UL = glGetUniformLocation(shaderProgID, "bDoNotLight");

	if (pCurrentObject->getIsWireframe())
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);		// LINES
		glUniform4f(debugColour_UL,
			pCurrentObject->getDebugColour().r,
			pCurrentObject->getDebugColour().g,
			pCurrentObject->getDebugColour().b,
			pCurrentObject->getDebugColour().a);
		glUniform1f(bDoNotLight_UL, (float)GL_TRUE);
	}
	else
	{	// Regular object (lit and not wireframe)
		glUniform1f(bDoNotLight_UL, (float)GL_FALSE);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);		// SOLID
	}
	//glPointSize(15.0f);

	if (pCurrentObject->getDisableDepthBufferTest())
	{
		glDisable(GL_DEPTH_TEST);					// DEPTH Test OFF
	}
	else
	{
		glEnable(GL_DEPTH_TEST);						// Turn ON depth test
	}

	if (pCurrentObject->getDisableDepthBufferWrite())
	{
		glDisable(GL_FALSE);						// DON'T Write to depth buffer
	}
	else
	{
		glEnable(GL_TRUE);								// Write to depth buffer
	}

	GLint isSkinnedMesh_UniLoc = glad_glGetUniformLocation(shaderProgID, "isSkinnedMesh");

	if (pCurrentObject->getpSm() != NULL)
	{
		glUniform1f(isSkinnedMesh_UniLoc, (float)GL_TRUE);

		// Set to all identity
		const int NUMBEROFBONES = 100;
		glm::mat4 matBones[NUMBEROFBONES];

		for (int index = 0; index != NUMBEROFBONES; index++)
		{
			matBones[index] = glm::mat4(1.0f);	// Identity
		}

		// Taken from "Skinned Mesh 2 - todo.docx"
		std::vector< glm::mat4x4 > vecFinalTransformation;
		std::vector< glm::mat4x4 > vecOffsets;
		std::vector< glm::mat4x4 > vecObjectBoneTransformation;

		// This loads the bone transforms from the animation model
		pCurrentObject->getpSm()->BoneTransform(HACK_FrameTime,	// 0.0f // Frame time
			currentAnimationName,
			vecFinalTransformation,
			vecObjectBoneTransformation,
			vecOffsets);

		// Wait until all threads are done updating.
		
		if (currentAnimationName == "Walk")
		{
			HACK_FrameTime += 0.003f;
		}
		else if (currentAnimationName == "Jump")
		{
			HACK_FrameTime += 0.003f;
		}
		else if (currentAnimationName == "Roll")
		{
			HACK_FrameTime += 0.004f;
		}
		else if (currentAnimationName == "Dying")
		{
			HACK_FrameTime += 0.001f;
		}
		else
		{
			HACK_FrameTime += 0.008f;
		}
		

		//{// Forward kinematic stuff

		//	// "Bone" location is at the origin
		//	glm::vec4 boneLocation = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

		//	// bone #22 is "B_R_Hand" in this model
		//	glm::mat4 matSpecificBone = vecFinalTransformation[9];

		//	// Transformed into "model" space where that bone is.
		//	::g_HACK_vec3_BoneLocationFK = matSpecificBone * boneLocation;

		//	// If it's in world space
		//	::g_HACK_vec3_BoneLocationFK = m * glm::vec4(g_HACK_vec3_BoneLocationFK, 1.0f);

		//	if (currentAnimationName == "Walk" || currentAnimationName == "Walk-Slow")
		//	{
		//		glm::vec3 setPosition = glm::vec3(0.0f, 0.0f, g_HACK_vec3_BoneLocationFK.z);

		//		pCurrentObject->setPositionXYZ(setPosition);
		//	}
		//}// Forward kinematic 


			// Copy all 100 bones to the shader
		GLint matBonesArray_UniLoc = glGetUniformLocation(shaderProgID, "matBonesArray[0]");
		// The "100" is to pass 100 values, starting at the pointer location of matBones[0];
		//glUniformMatrix4fv(matBonesArray_UniLoc, 100, GL_FALSE, glm::value_ptr(matBones[0]));

		GLint numBonesUsed = (GLint)vecFinalTransformation.size();

		glUniformMatrix4fv(matBonesArray_UniLoc, numBonesUsed,
			GL_FALSE,
			glm::value_ptr(vecFinalTransformation[0]));

	}
	else
	{
		glUniform1f(isSkinnedMesh_UniLoc, (float)GL_FALSE);
	}


	//		glDrawArrays(GL_TRIANGLES, 0, 2844);
	//		glDrawArrays(GL_TRIANGLES, 0, numberOfVertsOnGPU);

	sModelDrawInfo drawInfo;
	//if (pTheVAOManager->FindDrawInfoByModelName("bunny", drawInfo))
	if (pVAOManager->FindDrawInfoByModelName(pCurrentObject->getMeshName(), drawInfo))
	{
		glBindVertexArray(drawInfo.VAO_ID);
		glDrawElements(GL_TRIANGLES,
			drawInfo.numberOfIndices,
			GL_UNSIGNED_INT,
			0);
		glBindVertexArray(0);
	}

	return;
} // DrawObject;
// 

// returns NULL (0) if we didn't find it.
iObject* pFindObjectByFriendlyName(std::string name)
{
	// Do a linear search 
	for (unsigned int index = 0;
		index != g_vec_pGameObjects.size(); index++)
	{
		if (::g_vec_pGameObjects[index]->getFriendlyName() == name)
		{
			// Found it!!
			return ::g_vec_pGameObjects[index];
		}
	}
	for (unsigned int index = 0;
		index != g_vec_pEnvironmentObjects.size(); index++)
	{
		if (::g_vec_pEnvironmentObjects[index]->getFriendlyName() == name)
		{
			// Found it!!
			return ::g_vec_pEnvironmentObjects[index];
		}
	}
	for (unsigned int index = 0;
		index != g_vec_pGameFBOObjects.size(); index++)
	{
		if (::g_vec_pGameFBOObjects[index]->getFriendlyName() == name)
		{
			// Found it!!
			return ::g_vec_pGameFBOObjects[index];
		}
	}
	// Didn't find it
	return NULL;
}

// returns NULL (0) if we didn't find it.
iObject* pFindObjectByFriendlyNameMap(std::string name)
{
	//std::map<std::string, cGameObject*> g_map_GameObjectsByFriendlyName;
	return ::g_map_GameObjectsByFriendlyName[name];
}

glm::mat4 calculateWorldMatrix(iObject* pCurrentObject)
{

	glm::mat4 matWorld = glm::mat4(1.0f);


	// ******* TRANSLATION TRANSFORM *********
	glm::mat4 matTrans
		= glm::translate(glm::mat4(1.0f),
			glm::vec3(pCurrentObject->getPositionXYZ().x,
				pCurrentObject->getPositionXYZ().y,
				pCurrentObject->getPositionXYZ().z));
	matWorld = matWorld * matTrans;
	// ******* TRANSLATION TRANSFORM *********



	// ******* ROTATION TRANSFORM *********

	glm::mat4 rotation = glm::mat4(pCurrentObject->getRotationXYZ());
	matWorld *= rotation;
	// ******* ROTATION TRANSFORM *********



	// ******* SCALE TRANSFORM *********
	glm::mat4 scale = glm::scale(glm::mat4(1.0f),
		glm::vec3(pCurrentObject->getScale(),
			pCurrentObject->getScale(),
			pCurrentObject->getScale()));
	matWorld = matWorld * scale;
	// ******* SCALE TRANSFORM *********


	return matWorld;
}