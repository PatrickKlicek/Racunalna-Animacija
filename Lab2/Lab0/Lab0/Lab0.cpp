#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/random.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <fstream>
#include <sstream>
#include <SOIL2.h>

//
//OpenGL zahjteva da setupamo vertex i fragment shadere pisane u GLSL
//

// Vertex shader za èestice
const char* particleVertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"layout (location = 1) in vec2 aTexCoord;\n"
"out vec2 TexCoord;\n"
"uniform mat4 M;\n"
"uniform mat4 T;\n"
"uniform mat4 P;\n"
"void main()\n"
"{\n"
"   gl_Position = P * T * M * vec4(aPos, 1.0);\n"
"   TexCoord = aTexCoord;\n"
"}\0";

// Fragment shader za èestice
const char* particleFragmentShaderSource = "#version 330 core\n"
"in vec2 TexCoord;\n"
"out vec4 FragColor;\n"
"uniform sampler2D particleTexture;\n"
"void main()\n"
"{\n"
"   FragColor = texture(particleTexture, TexCoord);\n"
"}\n\0";

// Vertex shader
const char* vertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"layout (location = 1) in vec3 aColor;\n"
"out vec3 ourColor;\n"
"uniform mat4 T;\n"
"uniform mat4 P;\n"
"void main()\n"
"{\n"
"   gl_Position = P * T * vec4(aPos, 1.0);\n"
"   ourColor = aColor;\n"
"}\0";

// Fragment shader
const char* fragmentShaderSource = "#version 330 core\n"
"in vec3 ourColor;\n"
"out vec4 FragColor;\n"
"void main()\n"
"{\n"
"   FragColor = vec4(ourColor, 1.0f);\n"
"}\n\0";

/*glm::vec3 O(0.0f, 0.0f, 0.0f);
glm::vec3 G(0.0f, 0.0f, -4.0f);*/
glm::vec3 viewUp(0.0f, 1.0f, 0.0f);
glm::vec3 O, G;
glm::mat4 BSplineMatrica;

//
//Funkijca koja ce omoguciti da se prozor moze resizeati
//
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

//
//Funkcija za ucitavanje teksture
//
GLuint loadTexture(const char* filename) {
	GLuint texID = SOIL_load_OGL_texture(
		filename,
		SOIL_LOAD_AUTO,
		SOIL_CREATE_NEW_ID,
		SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y | SOIL_FLAG_NTSC_SAFE_RGB
	);

	if (texID == 0) {
		std::cout << "Greska pri ucitavanju teksture: " << SOIL_last_result() << std::endl;
	}
	return texID;
}

float brzinaStvaranja = 50.0f; //Parametar za koliko se cestica moze stvoriti u jednom render loopu

int temperatura = 10; //Za mjenjanje izmedju kise i snijega

float utjecajVjetraDesno = 0.0f; //Za utjecaj vjetra po x osi

float utjecajVjetraLijevo = 0.0f; //Za utjecaj vjetra po z osi
//
//Funkcija za proocesiranje inputa
//
static float angle = 0.0f;
float radius = 3.0f;
void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) //Provjerava se jeli tipka pritisnuta
		glfwSetWindowShouldClose(window, true); //Ako je ESCAPE pritisnut šalje se naredba za gašenje prozora
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) radius -= 0.05f;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) radius += 0.05f;
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) angle -= 0.02f;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) angle += 0.02f;
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) O.y -= 0.05f;
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) O.y += 0.05f;
	O.x = radius * cos(angle);
	O.z = radius * sin(angle);
	if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) brzinaStvaranja += 1;
	if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) brzinaStvaranja -= 1;
	if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) temperatura += 1;
	if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) temperatura -= 1;
	if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) utjecajVjetraDesno += 0.02;
	if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS) utjecajVjetraDesno -= 0.02;
	if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS) utjecajVjetraLijevo += 0.02;
	if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) utjecajVjetraLijevo -= 0.02;
}

//
//Struktura cestice
//

struct Cestica {
	glm::vec3 pozicija; //Gdje se nalazi cestica
	glm::vec3 smijer; //U kojem smijeru ide
	float brzina; //Kojom brzinom se krece
	float starost; //Zivot cestice
	float velicina; //Velicina cestice
	bool tip; //Kisa ili snijeg
};

float kisa = 1.0f; //Omjer kise
float snijeg = 0.0f; //Omjer snijega
int maxCestica = 1000; //Koliko maksimalno moze postojati cestica
std::vector<Cestica> cestice; //Vektor cestica

//
//Funkcija za stvaranje cestice
//
Cestica stvoriCesticu() {
	Cestica cestica;

	//Zadavanje pocetnog polozaja cestice
	cestica.pozicija = glm::vec3(
		glm::linearRand(-2.0f, 2.0f), //X oblaka
		-0.2f, //Malo ispod oblaka
		glm::linearRand(-1.0f, 1.0f) //Z oblaka
	);

	//Brzina i smijer kretanja
	glm::vec3 smijer = glm::vec3(glm::linearRand(-0.3f, 0.3f), 0.0f, glm::linearRand(-0.2f, 0.2f)); //Random smijer po x i z osi
	float brzina = glm::linearRand(1.5f, 3.5f); //Random brzina padanja
	cestica.brzina = brzina;
	cestica.smijer = glm::normalize(smijer + glm::vec3(0.0f, -1.0f, 0.0f)) * brzina; //Pošalji prema dolje (y = -1,0f) u onom random smjeru x i z osi
	cestica.starost = glm::linearRand(0.5f, 2.0f); //Random duljina zivota
	cestica.velicina = glm::linearRand(1.0f, 2.0f); //Random velicina cestice

	float random = glm::linearRand(0.0f, 1.0f); //Izaberi neki random broji
	
	//Ako je broj manji od postotka kise ova cestica je kisa
	if (random < kisa) {
		cestica.tip = 0;
	}

	//Inace je cestica snijega
	else {
		cestica.tip = 1;
	}

	return cestica;
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

	//
	//Shader program za cestice
	//
	unsigned int particleVertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(particleVertexShader, 1, &particleVertexShaderSource, NULL);
	glCompileShader(particleVertexShader);

	unsigned int particleFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(particleFragmentShader, 1, &particleFragmentShaderSource, NULL);
	glCompileShader(particleFragmentShader);

	unsigned int particleShaderProgram = glCreateProgram();
	glAttachShader(particleShaderProgram, particleVertexShader);
	glAttachShader(particleShaderProgram, particleFragmentShader);
	glLinkProgram(particleShaderProgram);

	glDeleteShader(particleVertexShader);
	glDeleteShader(particleFragmentShader);


	//Zadavanje vrhova oblaka
	float vertices[] = {
		//pozicija           //boja
		-1.5f, -0.5f, -0.5f, 0.678f, 0.678f, 0.678f,
		 1.5f, -0.5f, -0.5f, 0.678f, 0.678f, 0.678f,
		 1.5f,  0.5f, -0.5f, 0.678f, 0.678f, 0.678f,
		-1.5f,  0.5f, -0.5f, 0.678f, 0.678f, 0.678f,
		-1.5f, -0.5f,  0.5f, 0.678f, 0.678f, 0.678f,
		 1.5f, -0.5f,  0.5f, 0.678f, 0.678f, 0.678f,
		 1.5f,  0.5f,  0.5f, 0.678f, 0.678f, 0.678f,
		-1.5f,  0.5f,  0.5f, 0.678f, 0.678f, 0.678f,
	};

	//Posto opengl radi sa trokutima kazemo koji vrh da koristi za crtanje prvog i drugog trokuta kako bi dobili kvadar
	unsigned int indices[] = {
		0, 1, 2, 2, 3, 0, //straznja
		4, 5, 6, 6, 7, 4, //prednja
		0, 4, 7, 7, 3, 0, //lijeva
		1, 5, 6, 6, 2, 1, //desna
		3, 2, 6, 6, 7, 3, //gornja
		0, 1, 5, 5, 4, 0  //donja
	};

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
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); //Kopira definirane vertex podatke u buffer memoriju

	//glBufferData(GL_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	//glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	//glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

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
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0); //Pokreni te vertex atribute

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);


	//
	//Cestice kise
	//
	unsigned int kisaVAO, kisaVBO;
	glGenVertexArrays(1, &kisaVAO);
	glGenBuffers(1, &kisaVBO);
	glBindVertexArray(kisaVAO);
	glBindBuffer(GL_ARRAY_BUFFER, kisaVBO);
	glBufferData(GL_ARRAY_BUFFER, 2 * 6 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glBindVertexArray(0);


	//
	//Cestice snijega
	//
	float quadVertices[] = {
		//pos               // tex
		-0.1f, -0.1f, 0.0f, 0.0f, 0.0f,
		 0.1f, -0.1f, 0.0f, 1.0f, 0.0f,
		 0.1f,  0.1f, 0.0f, 1.0f, 1.0f,
		-0.1f,  0.1f, 0.0f, 0.0f, 1.0f
	};
	unsigned int quadIndices[] = {0,1,2,2,3,0};
	unsigned int quadVAO, quadVBO, quadEBO;
	glGenVertexArrays(1, &quadVAO);
	glGenBuffers(1, &quadVBO);
	glGenBuffers(1, &quadEBO);
	glBindVertexArray(quadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glBindVertexArray(0);

	//Ucitavanje slike
	GLuint myTexture = loadTexture("D:\\RACANIM\\Lab0\\x64\\Debug\\slika.png");
	glBindTexture(GL_TEXTURE_2D, myTexture);

	unsigned int cesticeVBO, cesticeVAO;
	glGenVertexArrays(1, &cesticeVAO);
	glGenBuffers(1, &cesticeVBO);
	glBindVertexArray(cesticeVAO);
	glBindBuffer(GL_ARRAY_BUFFER, cesticeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * maxCestica, nullptr, GL_DYNAMIC_DRAW); //Za 1000 cestica
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttrib3f(1, 1.0f, 1.0f, 1.0f);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);

	cestice.reserve(maxCestica);
	double vrijemeProslo = glfwGetTime(); //Dohvati vrijeme pokretanja programa
	double akumulatorStvaranja = 0.0f;

	for (int i = 0; i < 50; i++) cestice.push_back(stvoriCesticu());

	glEnable(GL_DEPTH_TEST);
	glViewport(0, 0, 800, 600); //Velicina prozora za renderanje, prva dva broja su koordinate donjeg lijevog kuta, a drugga dva su širina i visina
	
	//Za omoguciti transparentnost
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	while (!glfwWindowShouldClose(window)) //Render loop, kako se nebi uggasio program nakon što stvori sliku
	{
		//Dohvati vrijeme koje je proteklo i izracunaj razliku od zadnjeg render loopa
		double vrijemeTrenutno = glfwGetTime();
		float delta = float(vrijemeTrenutno - vrijemeProslo);
		vrijemeProslo = vrijemeTrenutno;

		//Izracun "temperature" i omjer kise i snijega
		if (temperatura > 4) {
			kisa = 1.0f;
			snijeg = 0.0f;
		}
		else if (temperatura <= 4 && temperatura >= 0) {
			kisa = temperatura / 4.0f;
			snijeg = 1.0f - kisa;
		}
		else if (temperatura < 0) {
			kisa = 0.0f;
			snijeg = 1.0f;
		}

		//input
		processInput(window); //Pozivanje funkcije za procesiranjje inputa svaki render loop (frame)

		//Azuriraj cestice
		for (auto& c : cestice)
		{
			c.starost -= delta; //Smanji zivot cestice
			c.smijer += glm::vec3(utjecajVjetraDesno, -6.0f, utjecajVjetraLijevo) * delta * c.brzina; //Odaberi smjer pomaka
			c.pozicija += c.smijer * delta; //Pomakni cesticu
		}

		//makni one cesitce kojima je isteklo vrijeme
		cestice.erase(
			std::remove_if(cestice.begin(), cestice.end(),[](const Cestica& c) { 
				return c.starost <= 0.0f; 
				}
			),cestice.end()
		);

		//Stvori nove cestice
		akumulatorStvaranja += brzinaStvaranja * delta;
		while (akumulatorStvaranja >= 1.0f && (int)cestice.size() < maxCestica) {
			cestice.push_back(stvoriCesticu());
			akumulatorStvaranja -= 1.0f;
		}

		//naredbe za renderanje
		glClearColor(0.133f, 0.133f, 0.133f, 1.0f); //Boja pozadine
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(shaderProgram); //koji shader program koristimo
		//
		//Projekcija
		//
		glm::mat4 T1 = glm::translate(glm::mat4(1.0f), -O);
		glm::mat4 Tz = glm::mat4(1.0f);
		Tz[2][2] = -1.0f;
		glm::vec4 GTmp(G, 1.0f);
		glm::vec4 G1Tmp = T1 * GTmp;
		glm::vec3 G1 = glm::vec3(G1Tmp);
		glm::vec3 zOs = glm::normalize(O - G);
		glm::vec3 xOs = glm::normalize(glm::cross(viewUp, zOs));
		glm::vec3 yOs = glm::normalize(glm::cross(zOs, xOs));
		glm::mat4 R(1.0f);
		R[0][0] = xOs.x; R[1][0] = xOs.y; R[2][0] = xOs.z;
		R[0][1] = yOs.x; R[1][1] = yOs.y; R[2][1] = yOs.z;
		R[0][2] = zOs.x; R[1][2] = zOs.y; R[2][2] = zOs.z;

		glm::mat4 T = R * T1;
		float H = glm::length(G - O);
		glm::mat4 P(0.0f);
		//debug ispis
		P = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
		std::cout << "O: (" << O.x << ", " << O.y << ", " << O.z << ")" << std::endl;
		std::cout << "H: " << H << std::endl;
		std::cout << "Temperatura: " << temperatura << std::endl;

		int TLoc = glGetUniformLocation(shaderProgram, "T");
		int PLoc = glGetUniformLocation(shaderProgram, "P");
		glUniformMatrix4fv(TLoc, 1, GL_FALSE, glm::value_ptr(T));
		glUniformMatrix4fv(PLoc, 1, GL_FALSE, glm::value_ptr(P));

		//
		//Crtanje "oblaka"
		//
		glBindVertexArray(VAO);

		glm::mat4 M1 = glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, 0.0f, 0.0f));
		M1 = glm::scale(M1, glm::vec3(1.0f, 0.6f, 1.0f));
		glUniformMatrix4fv(TLoc, 1, GL_FALSE, glm::value_ptr(T * M1));
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

		glm::mat4 M2 = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
		M2 = glm::scale(M2, glm::vec3(1.0f, 0.6f, 1.0f));
		glUniformMatrix4fv(TLoc, 1, GL_FALSE, glm::value_ptr(T * M2));
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

		glm::mat4 M3 = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		M3 = glm::scale(M3, glm::vec3(1.0f, 0.6f, 1.0f));
		glUniformMatrix4fv(TLoc, 1, GL_FALSE, glm::value_ptr(T * M3));
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

		glm::mat4 M4 = glm::translate(glm::mat4(1.0f), glm::vec3(0.5f, 0.0f, -0.5f));
		M4 = glm::scale(M4, glm::vec3(1.0f, 0.6f, 1.0f));
		glUniformMatrix4fv(TLoc, 1, GL_FALSE, glm::value_ptr(T * M4));
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

		glm::mat4 M5 = glm::translate(glm::mat4(1.0f), glm::vec3(-0.5f, 0.5f, 0.0f));
		M5 = glm::scale(M5, glm::vec3(0.8f, 0.5f, 0.8f));
		glUniformMatrix4fv(TLoc, 1, GL_FALSE, glm::value_ptr(T * M5));
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

		glm::mat4 M6 = glm::translate(glm::mat4(1.0f), glm::vec3(0.5f, 0.5f, 0.0f));
		M6 = glm::scale(M6, glm::vec3(0.8f, 0.5f, 0.8f));
		glUniformMatrix4fv(TLoc, 1, GL_FALSE, glm::value_ptr(T * M6));
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

		glm::mat4 M7 = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.5f, -0.5f));
		M7 = glm::scale(M7, glm::vec3(0.8f, 0.5f, 0.8f));
		glUniformMatrix4fv(TLoc, 1, GL_FALSE, glm::value_ptr(T * M7));
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

		glm::mat4 M8 = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.5f));
		M8 = glm::scale(M8, glm::vec3(1.0f, 0.6f, 1.0f));
		glUniformMatrix4fv(TLoc, 1, GL_FALSE, glm::value_ptr(T* M8));
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

		//
		//Crtanje cestica
		//
		for (auto& c : cestice) {
			//Cestice snijega
			if (c.tip == 1) {
				glUseProgram(particleShaderProgram);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, myTexture);
				glUniform1i(glGetUniformLocation(particleShaderProgram, "particleTexture"), 0);

				//Billboarding
				glm::mat4 M(1.0f);
				M[0] = glm::vec4(xOs * c.velicina, 0.0f);
				M[1] = glm::vec4(yOs * c.velicina, 0.0f);
				M[2] = glm::vec4(zOs * c.velicina, 0.0f);
				M[3] = glm::vec4(c.pozicija, 1.0f);

				int MLoc = glGetUniformLocation(particleShaderProgram, "M");
				glUniformMatrix4fv(MLoc, 1, GL_FALSE, glm::value_ptr(M));
				int TLoc = glGetUniformLocation(particleShaderProgram, "T");
				glUniformMatrix4fv(TLoc, 1, GL_FALSE, glm::value_ptr(T));
				int PLoc = glGetUniformLocation(particleShaderProgram, "P");
				glUniformMatrix4fv(PLoc, 1, GL_FALSE, glm::value_ptr(P));

				glBindVertexArray(quadVAO);
				glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
			}

			//Cestice kise
			else {
				glm::vec3 kraj = c.pozicija + glm::vec3(0.0f, -0.2f, 0.0f); //Izracun malo pomaknute tocke koja se spoji sa pocetnom kako bi se dobile "kapi"

				glUseProgram(shaderProgram);
				int TLoc = glGetUniformLocation(shaderProgram, "T");
				glUniformMatrix4fv(TLoc, 1, GL_FALSE, glm::value_ptr(T));
				int PLoc = glGetUniformLocation(shaderProgram, "P");
				glUniformMatrix4fv(PLoc, 1, GL_FALSE, glm::value_ptr(P));

				float lineVertices[] = {
					c.pozicija.x, c.pozicija.y, c.pozicija.z, 0.0f, 0.0f, 1.0f,
					kraj.x,       kraj.y,       kraj.z,       0.0f, 0.0f, 1.0f
				};

				glBindBuffer(GL_ARRAY_BUFFER, kisaVBO);
				glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(lineVertices), lineVertices);

				glBindVertexArray(kisaVAO);
				glDrawArrays(GL_LINES, 0, 2);
			}
		}

		//check i call eventi, buffeer swap
		glfwSwapBuffers(window); //Buffer za renderanje slike
		glfwPollEvents(); //Slusa za event triggere (tipkovnica ili miš)
	}

	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
	glDeleteVertexArrays(1, &cesticeVAO);
	glDeleteBuffers(1, &cesticeVBO);
	glDeleteVertexArrays(1, &kisaVAO);
	glDeleteBuffers(1, &kisaVBO);
	glDeleteProgram(shaderProgram);

	glfwTerminate(); //Èišèenje GLFW resursa
	return 0;
}