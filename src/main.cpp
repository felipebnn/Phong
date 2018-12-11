#include "phong.h"

static Phong p;
static bool running = true;

int main(int argc, char const *argv[]) {
	signal(SIGINT, [] (int) { p.killThreads(); running = false; });
	p.setThreadCount(std::thread::hardware_concurrency());

	if (argc == 1) {
		p.run("scene");
	} else {
		for (int i=1; i<argc && running; ++i) {
			std::string sceneName = argv[i];
			std::string extension = ".txt";

			if (std::equal(extension.rbegin(), extension.rend(), sceneName.rbegin())) {
				sceneName = sceneName.substr(0, sceneName.length() - 4);
			}

			size_t pos = sceneName.rfind('/');
			if (pos != std::string::npos) {
				sceneName = sceneName.substr(pos+1, std::string::npos);
			}

			p.run(sceneName);
		}
	}

}
