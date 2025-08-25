///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
	m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -5;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
		}
	}
}

//loading textures for the scene
void SceneManager::LoadSceneTextures()
{
	//loads image wood for wooden texture on desk
	CreateGLTexture("Textures/wood.jpg", "wood");
	//loads image wood for wooden texture on desk
	CreateGLTexture("Textures/keyboard.jpg", "keyboard");
	//loads image wood for wooden texture on desk
	CreateGLTexture("Textures/notebook.jpg", "notebook");
	//loads image wood for wooden texture on desk
	CreateGLTexture("Textures/mug.jpg", "mug");
	

	// Bind the loaded textures to texture units
	BindGLTextures();
}





void SceneManager::DefineObjectMaterials()
{
	// Glass material 
	OBJECT_MATERIAL glass;
	glass.tag = "glass";
	glass.diffuseColor = glm::vec3(0.10f, 0.10f, 0.10f); 
	glass.specularColor = glm::vec3(0.90f, 0.90f, 0.90f); 
	glass.shininess = 128.0f;                        
	glass.ambientColor = glm::vec3(0.60f, 0.60f, 0.60f); 
	glass.ambientStrength = 40.0f;
	m_objectMaterials.push_back(glass);
	//plastic material
	OBJECT_MATERIAL plastic;
	plastic.tag = "plastic";
	plastic.diffuseColor = glm::vec3(0.2f, 0.2f, 0.2f);   
	plastic.specularColor = glm::vec3(0.5f, 0.5f, 0.5f);   
	plastic.shininess = 80.0f;                      
	plastic.ambientColor = glm::vec3(0.15f, 0.15f, 0.15f);
	plastic.ambientStrength = 1.0f;
	m_objectMaterials.push_back(plastic);

	
}

void SceneManager::SetupSceneLights()
{
	m_pShaderManager->setBoolValue("bUseLighting", true);
	

	// put the point light around the top left of the keyboard/monitor to try and simulate sunlight
	m_pShaderManager->setBoolValue("pointLights[2].bActive", true);
	m_pShaderManager->setVec3Value("pointLights[2].position", glm::vec3(-1.20f, 1.00f, -1.20f)); 
	m_pShaderManager->setVec3Value("pointLights[2].ambient", glm::vec3(0.32f, 0.30f, 0.22f));
	m_pShaderManager->setVec3Value("pointLights[2].diffuse", glm::vec3(3.00f, 2.80f, 2.50f));
	m_pShaderManager->setVec3Value("pointLights[2].specular", glm::vec3(3.50f, 3.40f, 3.10f));

	


}
/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/


/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene
	//defining texture and object materials
	DefineObjectMaterials();
	LoadSceneTextures();

	m_basicMeshes->LoadPlaneMesh();//desk
	m_basicMeshes->LoadCylinderMesh(); // desk stand/mug
	m_basicMeshes->LoadBoxMesh();//notebooks
	m_basicMeshes->LoadConeMesh();//pencil tips/ hump for mouse
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadTorusMesh();//handle for mug

	
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{

	//setting up lights in the scene
	SetupSceneLights();
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.96f, 0.87f, 0.70f, 1.0f);//setting the desk to white
	SetShaderTexture("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/
	

	
	
	// Base disk for computer
	scaleXYZ = glm::vec3(0.70f, 0.05f, 0.70f);
	positionXYZ = glm::vec3(0.0f, 0.025f, -1.0f);
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();

	// Stand for computer
	scaleXYZ = glm::vec3(0.10f, 0.25f, 0.10f);
	positionXYZ = glm::vec3(0.0f, 0.175f, -1.0f);
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();

	// edges of the computer
	scaleXYZ = glm::vec3(1.80f, 0.50f, 0.06f); 
	positionXYZ = glm::vec3(0.0f, 0.55f, -1.0f);
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ);
	SetShaderColor(0.02f, 0.02f, 0.03f, 1.0f);
	SetShaderMaterial("glass");
	m_basicMeshes->DrawBoxMesh();

	// Screen of the computer
	scaleXYZ = glm::vec3(1.74f, 0.45f, 0.03f);
	positionXYZ = glm::vec3(0.0f, 0.550f, -0.98f);
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
	SetShaderMaterial("glass");
	m_basicMeshes->DrawBoxMesh();

	//keyboard	
	scaleXYZ = glm::vec3(1.6f, 0.05f, 0.45f);
	positionXYZ = glm::vec3(0.0f, 0.025f, 0.30f);
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ);
	SetShaderTexture("keyboard");
	m_basicMeshes->DrawBoxMesh();

	// Base of the mouse
	scaleXYZ = glm::vec3(0.22f, 0.05f, 0.30f);
	positionXYZ = glm::vec3(1.05f, 0.025f, 0.35f);
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
	SetShaderMaterial("plastic");
	m_basicMeshes->DrawBoxMesh();

	// Hump simulating the arch of a mouse
	scaleXYZ = glm::vec3(0.15f, 0.10f, 0.15f);
	positionXYZ = glm::vec3(1.05f, 0.100f, 0.35f);
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ);
	SetShaderMaterial("plastic");
	m_basicMeshes->DrawConeMesh();

	
	// Mug body 
	scaleXYZ = glm::vec3(0.20f, 0.35f, 0.20f);
	positionXYZ = glm::vec3(1.8f, 0.175f, -0.6f);
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ);
	SetShaderColor(0.5f, 0.5f, 0.5f, 1.0f); 
	m_basicMeshes->DrawCylinderMesh();

	// Rim of the mug
	scaleXYZ = glm::vec3(0.215f, 0.015f, 0.215f);
	positionXYZ = glm::vec3(1.80f, 0.3575f, -0.6f);
	SetTransformations(scaleXYZ,0, 0, 0, positionXYZ);
	SetShaderColor(0.5f, 0.5f, 0.5f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();

	// Handle of the mug
	scaleXYZ = glm::vec3(0.13f, 0.035f, 0.13f);
	XrotationDegrees = 180.0f; YrotationDegrees = 0.0f; ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(2.02f, 0.355f, -0.60f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.5f, 0.5f, 0.5f, 1.0f);
	m_basicMeshes->DrawTorusMesh();

	
	// Pencil 1 
	scaleXYZ = glm::vec3(0.03f, 0.5f, 0.03f);
	positionXYZ = glm::vec3(1.77f, 0.18f, -0.62f);
	SetTransformations(scaleXYZ, 0, 10.0f, 0, positionXYZ);
	SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();
	// Tip of pencil 1
	scaleXYZ = glm::vec3(0.03f, 0.4f, 0.03f);
	positionXYZ = glm::vec3(1.77f, 0.42f, -0.62f);
	SetTransformations(scaleXYZ, 0, 10.0f, 0, positionXYZ);
	SetShaderColor(0.30f, 0.20f, 0.15f, 1.0f);
	m_basicMeshes->DrawConeMesh();

	// Pencil 2
	scaleXYZ = glm::vec3(0.03f, 0.5f, 0.03f);
	positionXYZ = glm::vec3(1.835f, 0.175f, -0.585f);
	SetTransformations(scaleXYZ, 0, -8.0f, 0, positionXYZ);
	SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();
	//tip of pencil 2
	scaleXYZ = glm::vec3(0.03f, 0.4f, 0.03f);
	positionXYZ = glm::vec3(1.83f, 0.425f, -0.585f);
	SetTransformations(scaleXYZ, 0, -8.0f, 0, positionXYZ);
	SetShaderColor(0.30f, 0.20f, 0.15f, 1.0f);
	m_basicMeshes->DrawConeMesh();

	// Pencil 3
	scaleXYZ = glm::vec3(0.03f, 0.4f, 0.03f);
	positionXYZ = glm::vec3(1.75f, 0.178f, -0.555f);
	SetTransformations(scaleXYZ, 0, 4.0f, 0, positionXYZ);
	SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();
	//tip of pencil 3
	scaleXYZ = glm::vec3(0.03f, 0.5f, 0.03f);
	positionXYZ = glm::vec3(1.75f, 0.428f, -0.555f);
	SetTransformations(scaleXYZ, 0, 4.0f, 0, positionXYZ);
	SetShaderColor(0.30f, 0.20f, 0.15f, 1.0f);
	m_basicMeshes->DrawConeMesh();

	
	// Book 1 
	scaleXYZ = glm::vec3(0.40f, 0.07f, 0.60f);
	positionXYZ = glm::vec3(-2.60f, 0.035f, -0.20f);
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ);
	SetShaderColor(0.85f, 0.85f, 0.85f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// Book 2
	scaleXYZ = glm::vec3(0.42f, 0.08f, 0.58f);
	positionXYZ = glm::vec3(-2.10f, 0.04f, -0.18f);
	SetTransformations(scaleXYZ, 0, 2.5f, 0, positionXYZ);
	SetShaderColor(0.85f, 0.85f, 0.85f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// Book 3
	scaleXYZ = glm::vec3(0.38f, 0.06f, 0.62f);
	positionXYZ = glm::vec3(-1.7f, 0.03f, -0.22f);
	SetTransformations(scaleXYZ, 0, -6.0f, 0, positionXYZ);
	SetShaderColor(0.85f, 0.85f, 0.85f, 1.0f);
	m_basicMeshes->DrawBoxMesh();
}
	