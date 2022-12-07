#include <glm/glm.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <opencv2/opencv.hpp>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_stdlib.h"

#include <iostream>
#include <algorithm>
#include <utility>
#include <vector>
#include <fstream>

using namespace glm;

class Edge {
public:
	ImVec2 head;
	ImVec2 tail;
	bool drawing;

	Edge() {};
	Edge(ImVec2 h) : head(h), drawing(true){};
	Edge(ImVec2 h, ImVec2 t, bool d) : head(h), tail(t), drawing(d) {};

	void setTail(ImVec2 t) {
		tail = t;
		drawing = false;
	}
};

bool editHandle = false;
std::vector<Edge> sourceEdges;
std::vector<Edge> targetEdges;

void writeEdges(std::string sourcePath, std::string targetPath) {
	std::ofstream sourceLog("./source.edges");
	std::ofstream targetLog("./target.edges");
	
	for (auto edge : sourceEdges) {
		sourceLog << edge.head.x << " "
			<< edge.head.y << " "
			<< edge.tail.x << " "
			<< edge.tail.y << " "
			<< edge.drawing << std::endl;
	}

	for (auto edge : targetEdges) {
		targetLog << edge.head.x << " "
			<< edge.head.y << " "
			<< edge.tail.x << " "
			<< edge.tail.y << " "
			<< edge.drawing << std::endl;
	}
}

void readEdges() {
	std::ifstream sourceLog("./source.edges");
	std::ifstream targetLog("./target.edges");

	if (sourceLog.good()) {
		std::cout << "source edges:\n";
		while (!sourceLog.eof()) {
			Edge currEdge;
			sourceLog >> currEdge.head.x
				>> currEdge.head.y
				>> currEdge.tail.x
				>> currEdge.tail.y
				>> currEdge.drawing;
			if (!sourceLog.eof())
				sourceEdges.push_back(currEdge);
		}
	}
	

	if (targetLog.good()) {
		std::cout << "target edges:\n";
		while (!targetLog.eof()) {
			Edge currEdge;
			targetLog >> currEdge.head.x
				>> currEdge.head.y
				>> currEdge.tail.x
				>> currEdge.tail.y
				>> currEdge.drawing;
			if (!targetLog.eof())
				targetEdges.push_back(currEdge);
		}
	}
	std::cout << "edges read\n";
}

void printEdges() {

	std::cout << sourceEdges.size() << "\n";
	for (auto edge : sourceEdges) {
		std::cout << edge.head.x << " "
			<< edge.head.y << " "
			<< edge.tail.x << " "
			<< edge.tail.y << " "
			<< edge.drawing << std::endl;
	}

	std::cout << targetEdges.size() << "\n";
	for (auto edge : targetEdges) {
		std::cout << edge.head.x << " "
			<< edge.head.y << " "
			<< edge.tail.x << " "
			<< edge.tail.y << " "
			<< edge.drawing << std::endl;
	}
}

void loadImage(cv::Mat &image, std::string &buffer, GLuint &texture, const std::string &title) {
	ImGui::Begin(title.c_str());
	ImGui::InputText(("input##" + title).c_str(), &buffer, 512);

	if (ImGui::Button(("submit##" + title).c_str())) {
		image = cv::imread(buffer, cv::IMREAD_COLOR);
		if (!image.empty()) {
			cv::cvtColor(image, image, cv::COLOR_BGR2RGBA);
			glBindTexture(GL_TEXTURE_2D, texture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.cols, image.rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.data);
			cv::cvtColor(image, image, cv::COLOR_RGBA2BGR);
		}
	}
	
	if (ImGui::Button(("clear##" + title).c_str())) {
		if (title == "source path")
			sourceEdges.clear();
		else
			targetEdges.clear();
	}

	ImGui::End();
}

void displayImage(cv::Mat image, GLuint texture, std::string title) {
	std::vector<Edge> *edges = nullptr;
	if (title == "source image")
		edges = &sourceEdges;
	else
		edges = &targetEdges;

	ImGui::Begin(title.c_str());
	if (!image.empty()) {
		ImVec2 winSize = ImGui::GetWindowSize();
		double scaleFactor = std::min(winSize.y/ image.rows, winSize.x/ image.cols);	
		ImGui::Image(reinterpret_cast<void*>(static_cast<intptr_t>(texture)),
				ImVec2(image.cols * scaleFactor, image.rows * scaleFactor));
		ImVec2 topLeft = ImGui::GetItemRectMin(), bottomRight = ImGui::GetItemRectMax();
		
		if (editHandle) {
			ImGui::GetWindowDrawList()->AddRect(topLeft, bottomRight, IM_COL32(255, 0, 0, 255), 0, 0, 5);
			if (ImGui::IsItemClicked()){
				ImVec2 mousePos = ImGui::GetMousePos();
				if (edges->size() > 0 && edges->back().drawing)
					edges->back()
						.setTail(ImVec2((mousePos.x - topLeft.x) / scaleFactor, (mousePos.y - topLeft.y) / scaleFactor));
				else
					edges->push_back(
							Edge(ImVec2((mousePos.x - topLeft.x) / scaleFactor, (mousePos.y - topLeft.y) / scaleFactor))
					);
				
			}
		}
		
		for (int i = 0; i < edges->size(); i++) {
			ImVec2 head = ImVec2((*edges)[i].head.x*scaleFactor + topLeft.x, (*edges)[i].head.y*scaleFactor + topLeft.y);
			ImVec2 tail;

			if ((*edges)[i].drawing) {
				ImGui::GetWindowDrawList()->AddLine(head, ImGui::GetMousePos(), IM_COL32(255, 0, 0, 255), 2);
			}
			else {
				tail = ImVec2((*edges)[i].tail.x*scaleFactor + topLeft.x, (*edges)[i].tail.y*scaleFactor + topLeft.y);
				
				ImGui::SetCursorPos(ImVec2(tail.x - ImGui::GetWindowPos().x, tail.y - ImGui::GetWindowPos().y));
				ImGui::PushID(i);
				ImGui::Button("##tail", ImVec2(10, 10));
				ImGui::PopID();
				
				ImGui::GetWindowDrawList()->AddLine(head, tail, IM_COL32(255, 0, 0, 255), 2);
			}

			ImGui::SetCursorPos(ImVec2(head.x - ImGui::GetWindowPos().x, head.y - ImGui::GetWindowPos().y));
			ImGui::PushID(i);
			ImGui::Button("##head", ImVec2(10, 10));
			ImGui::PopID();

		}
	}
	ImGui::End();
}

float distance(vec3 x, vec3 p, vec3 q, float u, float v) {
	if (u >= 0 && u <= 1) {
		return std::abs(v);
	}
	if (u > 1) {
		return length(x - q);
	}
	if (u < 0) {
		return length(x - p);
	}
}

void clampToImage(vec3& p, const cv::Mat& image) {
	p.x = std::clamp((int)p.x, 2, image.cols - 2);
	p.y = std::clamp((int)p.y, 2, image.rows - 2);
}

void generateMorph(cv::Mat srcImg, cv::Mat tarImg) {
	std::cout << "generating morph\n";
	if (sourceEdges.size() != targetEdges.size()) {
		std::cout << "WARNING: Unequal number of edges in source and target !!\nCouldn't generate morph :(\n";
		return;
	}
	int tW = tarImg.cols, tH = tarImg.rows;
	int sW = srcImg.cols, sH = srcImg.rows;
	cv::Mat morphImg(sH, sW, tarImg.type(), cv::Scalar(0, 0, 0));
	for (int j = 0; j < tH; j++) {
		for (int i = 0; i < tW; i++) {
			// for each pixel (i, j)
			vec3 X = vec3(i, j, 0);
			float a = 50, b = 1, p = 0.2;
			vec3 DSUM = vec3(0, 0, 0);
			float weightsum = 0;
			for (int k = 0; k < sourceEdges.size(); k++) {
				Edge srcEdge = sourceEdges[k], tarEdge = targetEdges[k];
				// destination PQ
				vec3 P = vec3(tarEdge.head.x, tarEdge.head.y, 0);		
				vec3 Q = vec3(tarEdge.tail.x, tarEdge.tail.y, 0);
				// source P'Q'
				vec3 P1 = vec3(srcEdge.head.x, srcEdge.head.y, 0);		
				vec3 Q1 = vec3(srcEdge.tail.x, srcEdge.tail.y, 0);

				float u = dot((X - P), (Q - P)) / (length(Q - P) * length(Q - P));
				float v = dot((X - P), cross((Q - P), vec3(0, 0, 1))) / length(Q - P);
				vec3 X1i = P1 + u * (Q1 - P1) + (v * cross((Q1 - P1), vec3(0, 0, 1))) / length(Q1 - P1);
				vec3 Di = X1i - X;
				float dist = distance(X, P, Q, u, v);
				float weight = std::pow((std::pow(length(Q1 - P1), p) / (a + dist)), b);
				DSUM = DSUM + Di * weight;
				weightsum += weight;
			}
			vec3 X1 = X + DSUM / weightsum;
			// if (X1.x >= sW || X1.y >= sH || X1.x < 0 || X1.y < 0) {
			// 	continue;
			// }
			clampToImage(X1, srcImg);
			morphImg.at<cv::Vec3b>(j, i) = srcImg.at<cv::Vec3b>(X1.y, X1.x);
		}
	}
	imwrite("morph.png", morphImg);
	std::cout << "morph generated\n";
}

int main() {
	if (!glfwInit())
		return -1;

	const char* glsl_version = "#version 130";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

	GLFWwindow* window = glfwCreateWindow(500, 500, "image-morphing", NULL, NULL);

	if (!window) {
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);

	if (glewInit() != GLEW_OK)
		return -1;

	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);

	std::string sourcePathBuffer, targetPathBuffer;
	cv::Mat sourceImage, targetImage;
	sourcePathBuffer.reserve(512);
	targetPathBuffer.reserve(512);

	GLuint sourceTexture, targetTexture;
	glGenTextures(1, &sourceTexture);
	glGenTextures(1, &targetTexture);

	readEdges();

	while (!glfwWindowShouldClose(window)){

		glfwPollEvents();

		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGui::DockSpaceOverViewport();
		
		ImGui::ShowDemoWindow();

		loadImage(sourceImage, sourcePathBuffer, sourceTexture, "source path");
		displayImage(sourceImage, sourceTexture, "source image");

		loadImage(targetImage, targetPathBuffer, targetTexture, "target path");
		displayImage(targetImage, targetTexture, "target image");

		ImGui::Begin("Start Morphing");
		if (ImGui::Button("Morph")) {
			generateMorph(sourceImage, targetImage);
		}
		ImGui::End();

		if (ImGui::IsKeyPressed(ImGuiKey_G))
			editHandle = !editHandle;

		//Render
		ImGui::Render();
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(1.0, 1.0, 1.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		glfwSwapBuffers(window);
	}

	writeEdges(sourcePathBuffer, targetPathBuffer);

	glfwTerminate();

	return 0;
}