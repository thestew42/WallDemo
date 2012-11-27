/*----------------------------------------------------------------------------*\
|A test application class to demo the GLPipe and GLNode                        |
|                                                                              |
|Stewart Hall                                                                  |
|11/26/2012                                                                    |
\*----------------------------------------------------------------------------*/

#include "RGLInterface.h"

#include <stdio.h>

class App
{
private:
	//Filename to read configuration from
	char *configFile;

	//Address to connect to
	char *address;

	//Interface to remote OpenGL wall
	RGLInterface *rgl_interface;

public:
	App();
	~App();

	//Main loop of app
	int mainLoop();

	//Called when the program is run
	int run(int argc, char **argv);
};
