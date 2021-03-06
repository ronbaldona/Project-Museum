#include "Model.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

Model::Model()
{
}

Model::Model(const char* path) {
	loadModel(path);
}

Model::~Model()
{
}


GLint TextureFromFile(const char* path, string directory)
{
	//Generate texture ID and load texture data 
	string filename = string(path);
	filename = directory + '/' + filename;
	GLuint textureID;
	glGenTextures(1, &textureID);
	int width, height, nrChannels;
	unsigned char* image = stbi_load(filename.c_str(), &width, &height, &nrChannels, 0);
	// Assign texture to ID
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	glGenerateMipmap(GL_TEXTURE_2D);

	// Parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(image);
	return textureID;
}

void Model::loadModel(string path) {
	initTransformMat();
	Assimp::Importer import;
	const aiScene *scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		cout << "ERROR::ASSIMP::" << import.GetErrorString() << endl;
		return;
	}
	directory = path.substr(0, path.find_last_of('/'));

	// Initialize values for search
	xMax = FLT_MIN;
	xMin = FLT_MAX;
	yMax = FLT_MIN;
	yMin = FLT_MAX;
	zMax = FLT_MIN;
	zMin = FLT_MAX;

	processNode(scene->mRootNode, scene);

	// Resize
	float majorAxis;
	if (xMax - xMin > yMax - yMin && xMax - xMin > zMax - zMin)
		majorAxis = xMax - xMin;
	else if (yMax - yMin > zMax - zMin)
		majorAxis = yMax - yMin;
	else
		majorAxis = zMax - zMin;
	majorAxis = 1.0f / majorAxis;
	vec3 scaleVal = vec3(majorAxis, majorAxis, majorAxis);
	scale(scaleVal);
	// Move to center
	vec3 center = vec3(majorAxis * (xMin + xMax) / 2.0f, majorAxis * (yMin + yMax) / 2.0f, majorAxis * (zMin + zMin) / 2.0f);
	translate(-center);
}

void Model::processNode(aiNode *node, const aiScene *scene)
{
	// process all the node's meshes (if any)
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
		meshes.push_back(processMesh(mesh, scene));
	}
	// then do the same for each of its children
	for (unsigned int i = 0; i < node->mNumChildren; i++)
	{
		processNode(node->mChildren[i], scene);
	}
}

Mesh Model::processMesh(aiMesh *mesh, const aiScene *scene)
{
	vector<Vertex> vertices;
	vector<unsigned int> indices;
	vector<Texture> textures;

	for (unsigned int i = 0; i < mesh->mNumVertices; i++)
	{
		Vertex vertex;
		// process vertex positions, normals and texture coordinates
		vec3 vector;
		vector.x = mesh->mVertices[i].x;
		vector.y = mesh->mVertices[i].y;
		vector.z = mesh->mVertices[i].z;
		vertex.Pos = vector;
		
		if (xMax < vector.x)
			xMax = vector.x;
		else if (xMin > vector.x)
			xMin = vector.x;
		if (yMax < vector.y)
			yMax = vector.y;
		else if (yMin > vector.y)
			yMin = vector.y;
		if (zMax < vector.z)
			zMax = vector.z;
		else if (zMin > vector.z)
			zMin = vector.z;

		vector.x = mesh->mNormals[i].x;
		vector.y = mesh->mNormals[i].y;
		vector.z = mesh->mNormals[i].z;
		vertex.Normal = glm::normalize(vector);

		if (mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
		{
			glm::vec2 vec;
			vec.x = mesh->mTextureCoords[0][i].x;
			vec.y = mesh->mTextureCoords[0][i].y;
			vertex.texCoords = vec;
		}
		else
			vertex.texCoords = glm::vec2(0.0f, 0.0f);

		vertices.push_back(vertex);
	}

	// process indices
	for (unsigned int i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; j++)
			indices.push_back(face.mIndices[j]);
	}
	// process material
	if (mesh->mMaterialIndex >= 0)
	{
		aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
		vector<Texture> diffuseMaps = loadMaterialTextures(material,
			aiTextureType_DIFFUSE, "texture_diffuse");
		textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
		vector<Texture> specularMaps = loadMaterialTextures(material,
			aiTextureType_SPECULAR, "texture_specular");
		textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
	}

	return Mesh(vertices, indices, textures);
}

vector<Texture> Model::loadMaterialTextures(aiMaterial *mat, aiTextureType type, string typeName)
{
	vector<Texture> textures;
	for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
	{
		aiString str;
		mat->GetTexture(type, i, &str);
		bool skip = false;
		for (unsigned int j = 0; j < textures_loaded.size(); j++)
		{
			if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
			{
				textures.push_back(textures_loaded[j]);
				skip = true;
				break;
			}
		}
		if (!skip)
		{   // if texture hasn't been loaded already, load it
			Texture texture;
			texture.id = TextureFromFile(str.C_Str(), directory);
			texture.type = typeName;
			texture.path = str.C_Str();
			textures.push_back(texture);
			textures_loaded.push_back(texture); // add to loaded textures
		}
	}
	return textures;
}



void Model::setMaterialVal(const vec4 ambient, const vec4 diffuse, const vec4 specular, const vec4 emission,
						   const float shininess) {
	material.ambient = ambient;
	material.diffuse = diffuse;
	material.specular = specular;
	material.emission = emission;
	material.shininess = shininess;
}

void Model::setUniformMaterial(Shader shader) const {
	shader.setVec4("material.ambient", material.ambient);
	shader.setVec4("material.diffuse", material.diffuse);
	shader.setVec4("material.specular", material.specular);
	shader.setVec4("material.emission", material.emission);
	shader.setFloat("material.shininess", material.shininess);
}

void Model::translate(const float &tx, const float &ty, const float &tz) {
	mat4 transMat = mat4(1, 0, 0, 0,
			  		     0, 1, 0, 0,
						 0, 0, 1, 0,
						 tx, ty, tz, 1);
	this->transMat = this->transMat * transMat;

}
void Model::translate(const vec3 &tvec) {
	mat4 transMat = mat4(1, 0, 0, 0,
						 0, 1, 0, 0,
						 0, 0, 1, 0,
						 tvec.x, tvec.y, tvec.z, 1);
	this->transMat = this->transMat * transMat;

}
void Model::scale(const float &sx, const float &sy, const float &sz) {
	mat4 scaleMat = mat4(sx, 0, 0, 0,
						 0, sy, 0, 0,
						 0, 0, sz, 0,
						 0, 0, 0, 1);
	this->scaleMat = this->scaleMat * scaleMat;
}
void Model::scale(const vec3 &svec) {
	mat4 scaleMat = mat4(svec.x, 0, 0, 0,
						 0, svec.y, 0, 0,
						 0, 0, svec.z, 0,
						 0, 0, 0, 1);
	this->scaleMat = this->scaleMat * scaleMat;
}
void Model::rotate(const float degrees, const float ax, const float ay, const float az) {
	float rad = glm::radians(degrees);
	mat3 a_star = mat3(0, az, -ay,
					   -az, 0, ax,
					   ay, -ax, 0);
	mat3 a_at = mat3(ax * ax, ax * ay, ax * az,
					 ax * ay, ay * ay, ay * az,
					 ax * az, ay * az, az * az);
	mat3 tempRot = (cos(rad) * mat3()) + (a_at * (1 - cos(rad))) + (sin(rad) * a_star);

	mat4 rotMat = mat4(tempRot[0][0], tempRot[0][1], tempRot[0][2], 0,
					   tempRot[1][0], tempRot[1][1], tempRot[1][2], 0,
					   tempRot[2][0], tempRot[2][1], tempRot[2][2], 0,
					   0, 0, 0, 1);
	this->rotMat = this->rotMat * rotMat;

}
void Model::rotate(const float degrees, const vec3 & axis) {
	float rad = glm::radians(degrees);
	mat3 a_star = mat3(0, axis[2], -axis[1],
					   -axis[2], 0, axis[0],
					   axis[1], -axis[0], 0);
	mat3 a_at = mat3(axis[0] * axis[0], axis[0] * axis[1], axis[0] * axis[2],
					 axis[0] * axis[1], axis[1] * axis[1], axis[1] * axis[2],
					 axis[0] * axis[2], axis[1] * axis[2], axis[2] * axis[2]);


	mat3 tempRot = (cos(rad) * mat3()) + (a_at * (1 - cos(rad))) + (sin(rad) * a_star);
	mat4 rotMat = mat4(tempRot[0][0], tempRot[0][1], tempRot[0][2], 0,
					   tempRot[1][0], tempRot[1][1], tempRot[1][2], 0,
					   tempRot[2][0], tempRot[2][1], tempRot[2][2], 0,
					   0, 0, 0, 1);
	this->rotMat = rotMat * this->rotMat;
}

void Model::getAxisAngle(float& angle, vec3& axis, vec3 v1, vec3 v2) {
	v1 = glm::normalize(v1);
	v2 = glm::normalize(v2);
	axis = glm::cross(v1, v2);
	angle = acos(glm::dot(v1, v2)) * (180.0f / PI);
}

void Model::Draw(Shader shader, const mat4& view, const mat4& projection) {
	mat4 model = transMat * rotMat * scaleMat;

	// Send the material info over to the shader
	setUniformMaterial(shader);
	shader.setMat4("view", view);
	shader.setMat4("modelview", view * model);
	shader.setMat4("projection", projection);
	for (unsigned int i = 0; i < meshes.size(); i++)
		meshes[i].Draw(shader);
}