#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <fstream>
#include <sstream>

//
//OpenGL zahjteva da setupamo vertex i fragment shadere pisane u GLSL
//
const char* vertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"uniform mat4 T;\n"
"uniform mat4 P;\n"
"void main()\n"
"{\n"
"   gl_Position = P * T * vec4(aPos, 1.0);\n"
"}\0";
const char* fragmentShaderSource = "#version 330 core\n"
"out vec4 FragColor;\n"
"void main()\n"
"{\n"
"   FragColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);\n" //Boja trokuta
"}\n\0";

/*glm::vec3 O(0.0f, 0.0f, 0.0f);
glm::vec3 G(0.0f, 0.0f, -4.0f);*/
glm::vec3 viewUp(0.0f, 1.0f, 0.0f);
glm::vec3 O, G;
glm::mat4 BSplineMatrica;

//
//Putanja u obliku spirale
//
std::vector<glm::vec3> spirala = {
	{0.0f, 0.0f,  0.0f},
	{0.0f, 10.0f, 5.0f},
	{10.0f, 10.0f, 10.0f},
	{10.0f, 0.0f, 15.0f},
	{0.0f, 0.0f, 20.0f},
	{0.0f, 10.0f, 25.0f},
	{10.0f, 10.0f, 30.0f},
	{10.0f, 0.0f, 35.0f},
	{0.0f, 0.0f, 40.0f},
	{0.0f, 10.0f, 45.0f},
	{10.0f, 10.0f, 50.0f},
	{10.0f, 0.0f, 55.0f}
};

//
//Funkijca koja ce omoguciti da se prozor moze resizeati
//
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

//
//Funkcija za proocesiranje inputa
//
void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) //Provjerava se jeli tipka pritisnuta
		glfwSetWindowShouldClose(window, true); //Ako je ESCAPE pritisnut šalje se naredba za gašenje prozora
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) O.z -= 0.05f;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) O.z += 0.05f;
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) O.x -= 0.05f;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) O.x += 0.05f;
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) O.y -= 0.05f;
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) O.y += 0.05f;
	if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) G.z -= 0.05f;
	if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) G.z += 0.05f;
	if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) G.x -= 0.05f;
	if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) G.x += 0.05f;
	if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS) G.y -= 0.05f;
	if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) G.y += 0.05f;
}

//
//Aproksimacijska kubna B-splajn krivulja odreðena sa 4 toèke
//
/*const glm::mat4 BSplineMatrica = glm::transpose(glm::mat4(
	-1.0f, 3.0f, -3.0f, 1.0f,
	3.0f, -6.0f, 3.0f, 0.0f,
	-3.0f, 0.0f, 3.0f, 0.0f,
	1.0f, 4.0f, 1.0f, 0.0f
));*/

//
//Ucitaj koordinate
//
void ucitajKoordinate(const std::string& path, glm::vec3& O, glm::vec3& G, glm::mat4& BSplineMatrica) {
	std::ifstream file(path);
	std::string line;
	float x, y, z, w;

	//O
	std::getline(file, line);
	std::istringstream(line) >> x >> y >> z;
	O = glm::vec3(x, y, z);

	//G
	std::getline(file, line);
	std::istringstream(line) >> x >> y >> z;
	G = glm::vec3(x, y, z);

	//Matrica
	for (int red = 0; red < 4; red++) {
		std::getline(file, line);
		std::istringstream(line) >> BSplineMatrica[red][0] >> BSplineMatrica[red][1]
			>> BSplineMatrica[red][2] >> BSplineMatrica[red][3];
	}
}

//
//Funkcija za odreðivanje segmenta i uz parametar t
//Parametar t se mijenja u rasponu 0 <= t <= 1
//Sa 4 toèke odreðen je jean segment krivulje, a opcenito s n tocaka je odreðeno n-3 segmenata kirvulje
//
glm::vec3 BSplineSegment(const std::vector<glm::vec3>& R, int i, float t) {
	//T = [t^3 t^2 t 1]
	glm::vec4 vektorT(t * t * t, t * t, t, 1);

	glm::vec4 T3Bi3 = vektorT * BSplineMatrica;

	glm::vec3 p(0.0f);
	for (int j = 0; j < 4; j++) {
		p += T3Bi3[j] * R[i + j];
	}
	return p * (1.0f / 6.0f);
}

//
//Funkcija za izracun tangente na aproksimacijsku uniformnu B-spline ubnu krrivulju
//
glm::vec3 BSPlineTangenta(const std::vector<glm::vec3>& R, int i, float t) {
	//T = [3*t*t 2*t 1 0]
	glm::vec4 vektorT(3 * t * t, 2 * t, 1, 0);

	glm::vec4 TBi3 = vektorT * BSplineMatrica;

	glm::vec3 p(0.0f);
	for (int j = 0; j < 4; j++) {
		p += TBi3[j] * R[i + j];
	}

	return p * (1.0f / 6.0f);
}

//
//Funkcija za izracun druge derivacije (derivacija tangente)
//
glm::vec3 BSPlineTangentaDerivacija(const std::vector<glm::vec3>& R, int i, float t) {
	//T = [6*t 2 0 0]
	glm::vec4 vektorT(6 * t, 2, 0, 0);

	glm::vec4 TBi3 = vektorT * BSplineMatrica;

	glm::vec3 p(0.0f);
	for (int j = 0; j < 4; j++) {
		p += TBi3[j] * R[i + j];
	}

	return p * (1.0f / 6.0f);
}

//
//Ucitavanje modela
//
void ucitajModel(const std::string& path, std::vector<float>& vertices, std::vector<unsigned int>& indices) {
	Assimp::Importer importer;

	const aiScene* scene = importer.ReadFile(path,
		aiProcess_Triangulate |
		aiProcess_GenNormals |
		aiProcess_JoinIdenticalVertices |
		aiProcess_OptimizeMeshes |
		aiProcess_PreTransformVertices
	);

	aiMesh* mesh = scene->mMeshes[0];

	//Vrhovi
	for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
		aiVector3D pos = mesh->mVertices[i];
		vertices.push_back(pos.x);
		vertices.push_back(pos.y);
		vertices.push_back(pos.z);
	}

	//Indeksi
	for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
		aiFace face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; j++) {
			indices.push_back(face.mIndices[j]);
		}
	}
}


int main() {
	glfwInit(); //Inicijalizacija GLFW
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); //Konfiguracija majore verzije OpenGl-a 3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3); //Konfiguracija minore verzije OpenGl-a 3
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); //Pristup manjem subsetu OpenGl funkcija

	GLFWwindow* window = glfwCreateWindow(800, 600, "Labos", NULL, NULL); //Stvarannje prozora, prvo se definira širina i visina, zatim ime
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback); //Pozivanje funkcije za svaki resize

	//
	//Inicijalizacija GLAD jer on upravvlja pointerima za OpenGL
	//
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	//glEnable(GL_DEPTH_TEST);
	ucitajKoordinate("C:/Users/Patrick/Desktop/model/data.txt", O, G, BSplineMatrica);
	BSplineMatrica = glm::transpose(BSplineMatrica);

	//
	//Stvaranje vertex shadera
	//
	unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER); //Stvaranje shadera i pohrana kao unsigned int
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL); //Stavljamo shader source code na shader objekt 
	glCompileShader(vertexShader); //Kompjlamo shader

	//Provjera jeli kompajliranje shadera bilo uspješno
	int success;
	char infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
	}

	//
	//Stvaranje fragment shadera (shader za booju)
	//
	unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER); //Stvaranje shadera i pohrana kao unsigned int
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL); //Stavljamo shader source code na shader objket
	glCompileShader(fragmentShader); //Kompjalnje shadera

	//Provjera jeli kompjalanje shadera bilo uspješno
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
	}

	//
	//Linkanje shadera
	//
	unsigned int shaderProgram = glCreateProgram(); //shader prrogram je objekt koji linka više shadera zajedno
	glAttachShader(shaderProgram, vertexShader); //dodaj shader program sa vertex shaderom
	glAttachShader(shaderProgram, fragmentShader); //dodaj shader program sa fragment shaderom
	glLinkProgram(shaderProgram); //Linkaj sve dodane shadere na shader program

	//Provjera jeli linkanje bilo uspjesno
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
	}

	glDeleteShader(vertexShader); //Nakon linkanja nije više potrebno
	glDeleteShader(fragmentShader); //Nakon linkanja nije više potrebno

	/*//Zadavanje vrhova trokuta
	float vertices[] = {
		// prednja strana
		-0.5f, -0.5f,  0.5f,
		 0.5f, -0.5f,  0.5f,
		 0.5f,  0.5f,  0.5f,
		-0.5f,  0.5f,  0.5f,
		// stražnja strana
		-0.5f, -0.5f, -0.5f,
		 0.5f, -0.5f, -0.5f,
		 0.5f,  0.5f, -0.5f,
		-0.5f,  0.5f, -0.5f
	};

	//Zadavanje vrhova i njihovo koristenje za crtanje kvadra
	float vertices2[] = {
	 0.5f,  0.5f, 0.0f,
	 0.5f, -0.5f, 0.0f,
	-0.5f, -0.5f, 0.0f,
	-0.5f,  0.5f, 0.0f
	};

	//Posto opengl radi sa trokutima kazemo koji vrh da koristi za crtanje prvog i drugog trokuta kako bi dobili kvadar
	unsigned int indices[] = {
		// prednja strana
		0, 1, 2, 2, 3, 0,
		// stražnja strana
		4, 5, 6, 6, 7, 4,
		// lijeva strana
		4, 0, 3, 3, 7, 4,
		// desna strana
		1, 5, 6, 6, 2, 1,
		// gornja strana
		3, 2, 6, 6, 7, 3,
		// donja strana
		4, 5, 1, 1, 0, 4
	};*/
	std::vector<float> vertices;
	std::vector<unsigned int> indices;

	ucitajModel("C:/Users/Patrick/Desktop/model/teddy.obj", vertices, indices);

	float scale = 0.1f;
	for (size_t i = 0; i < vertices.size(); i += 3) {
		vertices[i + 0] *= scale;
		vertices[i + 1] *= scale;
		vertices[i + 2] *= scale;
	}

	//
	//----------------------------------------------
	//VBO = vvertex buffer objects za manageiranje memorije GPU gdje šaljom vrhove
	//VAO = vertex array object, potreban kako bi opengl znnao što sa inputom inace odbija crtati
	//EBO = element buffer objects
	//
	unsigned int VBO, VAO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO); //spoji VAO, a tek onda VBO i onda tek atribute postavi

	glBindBuffer(GL_ARRAY_BUFFER, VBO); //spoji VBO, buffer call se koristi za koonfiguraciju VBO
	//glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); //Kopira definirane vertex podatke u buffer memoriju
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW); //Kopira definirane vertex podatke u buffer memoriju

	//glBufferData(GL_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	//glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	//glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

	//
	//Potrebno definirati koji dio unosnih podataka ide kojem vertexu
	//Moramo specificirati kako OpenGL interpretira podatke 
	//Kazemo kako da interpretira podatke OpenGL, 
	// 1. lokacija position vertexa, 
	// 2. velicina vertex podataka (vec3 pa 3), 
	// 3. tip podataka
	// 4. zelimo li da se normaliziraju podaci?
	// 5. razmak izmeðu vertež atributa
	//
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0); //Pokreni te vertex atribute
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	//----------------------------------------------

	glEnable(GL_DEPTH_TEST);

	//
	//---------------------------------------------- 
	//VBO i VAO za krivulju
	//
	unsigned int krivuljaVBO, krivuljaVAO;
	glGenVertexArrays(1, &krivuljaVAO);
	glGenBuffers(1, &krivuljaVBO);

	glBindVertexArray(krivuljaVAO);

	glBindBuffer(GL_ARRAY_BUFFER, krivuljaVBO);
	glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);
	//----------------------------------------------


	//
	//----------------------------------------------
	//VBO i VAO za tangente
	//
	unsigned int tangentaVBO, tangentaVAO;
	glGenVertexArrays(1, &tangentaVAO);
	glGenBuffers(1, &tangentaVBO);

	glBindVertexArray(tangentaVAO);

	glBindBuffer(GL_ARRAY_BUFFER, tangentaVBO);
	glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);
	//----------------------------------------------


	//Sa 4 toèke odreðen je jedan segment krivulje, a opcenito s n tocaka je odreðeno n-3 segmenata kirvulje
	int brSegmenata = (int)spirala.size() - 3;

	//Varijabla za mijenjanje vrijednosti t po segmentu od 0 do 1
	const int mijenjajParametart = 25;

	//
	//Parametri za aniamciju
	//
	float putanjaKocke = 0.0f;
	float pomakKocke = 0.005f;

	glViewport(0, 0, 800, 600); //Velicina prozora za renderanje, prva dva broja su koordinate donjeg lijevog kuta, a drugga dva su širina i visina

	while (!glfwWindowShouldClose(window)) //Render loop, kako se nebi uggasio program nakon što stvori sliku
	{
		//input
		processInput(window); //Pozivanje funkcije za procesiranjje inputa svaki render loop (frame)

		//Pomak kocke
		putanjaKocke += pomakKocke;
		float segmentKocke = fmod(putanjaKocke, (float)brSegmenata); //Koji segment krivulje je kocka prosla
		int indeksSegmentaKocke = (int)floor(segmentKocke);
		float kockaT = segmentKocke - float(indeksSegmentaKocke); //t u rasponeu od 0 do 1 za kretannje kocke


		//
		//Projekcija
		//
		glm::mat4 T1 = glm::translate(glm::mat4(1.0f), -O); //pomak ishodista kord. sustava oka u ishodiste scene
		/*std::cout << "T1 matrica \n";
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				std::cout << T1[i][j] << "\t";
			}
			std::cout << "\n";
		}*/
		glm::mat4 Tz = glm::mat4(1.0f);
		Tz[2][2] = -1.0f;
		glm::vec4 GTmp(G, 1.0f);
		glm::vec4 G1Tmp = T1 * GTmp;
		glm::vec3 G1 = glm::vec3(G1Tmp);
		glm::vec3 zOs = glm::normalize(G - O);
		glm::vec3 xOs = glm::normalize(glm::cross(glm::normalize(viewUp), zOs));
		glm::vec3 yOs = glm::normalize(glm::cross(zOs, xOs));
		glm::mat4 Ruku(1.0f);
		Ruku[0][0] = xOs.x; Ruku[1][0] = yOs.x; Ruku[2][0] = zOs.x;
		Ruku[0][1] = xOs.y; Ruku[1][1] = yOs.y; Ruku[2][1] = zOs.y;
		Ruku[0][2] = xOs.z; Ruku[1][2] = yOs.z; Ruku[2][2] = zOs.z;
		glm::mat4 T = T1 * Ruku * Tz; //Ukupna matrica transformacije
		float H = glm::length(G - O); //Udaljenost ravnine projekcije od ocista
		glm::mat4 P(0.0f);
		P[0][0] = 1.0f;
		P[1][1] = 1.0f;
		P[2][3] = 1.0f;
		P[3][2] = 1.0f / H;
		//debug ispis
		std::cout << "O: (" << O.x << ", " << O.y << ", " << O.z << ")" << std::endl;
		std::cout << "H: " << H << std::endl;

		//
		//----------------------------------------------
		//Stvaranje krivulje
		//
		std::vector<glm::vec3> tockeKrivulje;
		std::vector<glm::vec3> tangenteKrivulje;
		for (int segment = 0; segment < brSegmenata; segment++) {
			for (int uzorkovanje = 0; uzorkovanje <= mijenjajParametart; uzorkovanje++) {
				float t = (float)uzorkovanje / (float)mijenjajParametart; //Uzorkovanje parametra t od 0 do 1
				glm::vec3 translacija = BSplineSegment(spirala, segment, t);
				glm::vec3 tangenta = BSPlineTangenta(spirala, segment, t);
				tockeKrivulje.push_back(translacija);
				tangenteKrivulje.push_back(translacija);
				tangenteKrivulje.push_back(translacija + glm::normalize(tangenta) * 3.0f);
			}
		}
		//----------------------------------------------


		glBindBuffer(GL_ARRAY_BUFFER, krivuljaVBO);
		glBufferData(GL_ARRAY_BUFFER, tockeKrivulje.size() * 3 * sizeof(float), tockeKrivulje.data(), GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, tangentaVBO);
		glBufferData(GL_ARRAY_BUFFER, tangenteKrivulje.size() * 3 * sizeof(float), tangenteKrivulje.data(), GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		//naredbe za renderanje
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f); //Boja pozadine
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//
		//crtanje
		//
		glUseProgram(shaderProgram); //koji shader program koristimo
		int TLoc = glGetUniformLocation(shaderProgram, "T");
		int PLoc = glGetUniformLocation(shaderProgram, "P");
		glUniformMatrix4fv(TLoc, 1, GL_FALSE, glm::value_ptr(T));
		glUniformMatrix4fv(PLoc, 1, GL_FALSE, glm::value_ptr(P));

		//Crtanje krivulje
		glBindVertexArray(krivuljaVAO);
		glDrawArrays(GL_LINE_STRIP, 0, (GLsizei)tockeKrivulje.size());
		glBindVertexArray(0);

		//Crtanje tangenti
		glBindVertexArray(tangentaVAO);
		glDrawArrays(GL_LINES, 0, (GLsizei)tangenteKrivulje.size());
		glBindVertexArray(0);


		//
		//Odrediti ciljnu orijentaciju objekta i transalciju objekta
		//

		//pomak
		glm::vec3 pozicijaKocke = BSplineSegment(spirala, indeksSegmentaKocke, kockaT);

		//ciljna orijentacija
		glm::vec3 e = BSPlineTangenta(spirala, indeksSegmentaKocke, kockaT);

		//pocetna orijentacija
		static glm::vec3 s = glm::vec3(0.0f, 1.0f, 0.0f);

		//os rotacije s x e
		glm::vec3 osRotacije = glm::cross(s, e);

		//kosinus kuta rotacije
		float cosTheta = glm::dot(s, e) / (glm::length(s) * glm::length(e));
		float theta = acosf(cosTheta);

		//matrica rotacije
		glm::mat4 R = glm::rotate(glm::mat4(1.0f), theta, osRotacije);

		glm::mat4 zarotiranaKocka = glm::translate(glm::mat4(1.0f), pozicijaKocke) * R;

		glm::mat4 Ttotal = T * zarotiranaKocka;
		glUniformMatrix4fv(TLoc, 1, GL_FALSE, glm::value_ptr(Ttotal));
		glUniformMatrix4fv(PLoc, 1, GL_FALSE, glm::value_ptr(P));

		//Crtanje kocke
		glBindVertexArray(VAO); //sto sa inputom
		//glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
		glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, 0);
		//glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);


		//
		//Odrediti ciljnu orijentaciju objekta i transalciju objekta (preko DCM)
		//
		/*
		glm::mat4 Rinicijalno = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		glm::vec3 pozicijaKocke2 = BSplineSegment(spirala, segmentKocke, kockaT);
		glm::vec3 w = BSPlineTangenta(spirala, segmentKocke, kockaT); //vektor tangennte
		w = glm::normalize(w);
		glm::vec3 drugaDerivacijaKocke = BSPlineTangentaDerivacija(spirala, segmentKocke, kockaT);
		glm::vec3 u = glm::cross(w, drugaDerivacijaKocke); //vektor normale
		if (glm::length(u) < 0.001f) {
			u = glm::cross(w, viewUp);
		}
		u = glm::normalize(u);
		glm::vec3 v = glm::cross(w, u); //vektor binormale
		v = glm::normalize(v);

		//DCM mastrica rotacije
		glm::mat4 R2(1.0f);
		R2[0][0] = w.x; R2[1][0] = u.x; R2[2][0] = v.x;
		R2[0][1] = w.y; R2[1][1] = u.y; R2[2][1] = v.y;
		R2[0][2] = w.z; R2[1][2] = u.z; R2[2][2] = v.z;

		glm::mat4 zarotiranaKocka2 = glm::translate(glm::mat4(1.0f), pozicijaKocke2) * R2 * Rinicijalno;

		glm::mat4 Ttotal2 = T * zarotiranaKocka2;
		glUniformMatrix4fv(TLoc, 1, GL_FALSE, glm::value_ptr(Ttotal2));
		glUniformMatrix4fv(PLoc, 1, GL_FALSE, glm::value_ptr(P));

		//Crtanje kocke
		glBindVertexArray(VAO); //sto sa inputom
		//glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
		glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, 0);
		//glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);*/


		//check i call eventi, buffeer swap
		glfwSwapBuffers(window); //Buffer za renderanje slike
		glfwPollEvents(); //Slusa za event triggere (tipkovnica ili miš)
	}

	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
	glDeleteProgram(shaderProgram);
	glDeleteVertexArrays(1, &krivuljaVAO);
	glDeleteBuffers(1, &krivuljaVBO);
	glDeleteVertexArrays(1, &tangentaVAO);
	glDeleteBuffers(1, &tangentaVBO);

	glfwTerminate(); //Èišèenje GLFW resursa
	return 0;
}

