/*----------------------------------------------------------------------------*\
|A test application class to demo the RGLInterface and GLNode                  |
|                                                                              |
|Stewart Hall                                                                  |
|11/26/2012                                                                    |
\*----------------------------------------------------------------------------*/

#include "App.h"

//Constructor
App::App()
{
	configFile = NULL;
}

//Destructor
App::~App()
{

}

//Initializes application and starts up graphics
int App::run(int argc, char **argv)
{
	//Parse arguments
	for(int i = 0; i < argc; i++) {
		if(argv[i][0] == '-' && argv[i][1] == 'c') {
			//Config file argument
			i++;
			configFile = argv[i];
		} else if(argv[i][0] == '-' && argv[i][1] == 'a') {
			//Visualization wall address
			i++;
			address = argv[i];
		}
	}

	//Set up the remote OpenGL interface given the config file
	if(!configFile)
		return -1;

	rgl_interface = new RGLInterface(configFile);

	//Connect to all nodes
	if(rgl_interface->initialize(address) < 0)
		return -1;

	//Start main loop
	mainLoop();

	//Wait for input
	printf("Application successfully initialized. Press any key to clean up.\n");
	getchar();

	//Close all connections
	rgl_interface->cleanUp();

	delete rgl_interface;

	return 0;
}

int App::mainLoop()
{
	float rtri = 0.0f;
	rgl_interface->glClearColor(0.0f, 0.0f, 0.3f, 0.5f);

	//Main render loop
	for(;;) {
		//Clear the color and depth buffer
		rgl_interface->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		//Set up model transform matrix
		rgl_interface->glLoadIdentity();
		rgl_interface->glTranslatef(0.0f, 0.0f, -7.0f);
		rgl_interface->glRotatef(rtri, 0.0f, 1.0f, 0.0f);
		rgl_interface->glScalef(3.0f, 3.0f, 3.0f);

		//Draw the shape
		rgl_interface->glBegin(GL_TRIANGLES);
			rgl_interface->glColor3f(1.0f,0.0f,0.0f);
			rgl_interface->glVertex3f( 0.0f, 1.0f, 0.0f);
			rgl_interface->glColor3f(0.0f,1.0f,0.0f);
			rgl_interface->glVertex3f(-1.0f,-1.0f, 1.0f);
			rgl_interface->glColor3f(0.0f,0.0f,1.0f);
			rgl_interface->glVertex3f( 1.0f,-1.0f, 0.0f);

			rgl_interface->glColor3f(1.0f,0.0f,0.0f);
			rgl_interface->glVertex3f( 0.0f, 1.0f, 0.0f);
			rgl_interface->glColor3f(0.0f,1.0f,0.0f);
			rgl_interface->glVertex3f( 1.0f,-1.0f, 0.0f);
			rgl_interface->glColor3f(0.0f,0.0f,1.0f);
			rgl_interface->glVertex3f(-1.0f,-1.0f, -1.0f);
   
			rgl_interface->glColor3f(1.0f,0.0f,0.0f);
			rgl_interface->glVertex3f( 0.0f, 1.0f, 0.0f);
			rgl_interface->glColor3f(0.0f,0.0f,1.0f);
			rgl_interface->glVertex3f(-1.0f,-1.0f,-1.0f);
			rgl_interface->glColor3f(0.0f,1.0f,0.0f);
			rgl_interface->glVertex3f(-1.0f,-1.0f, 1.0f);
		rgl_interface->glEnd();

		//Frame rendering done, send sync packet
		rgl_interface->sendSync();

		//Sleep between frames and rotate shape
		rtri += 0.2f;
	}

	return 0;
}

//Entry point
int main(int argc, char *argv[])
{
	App *app = new App();
	app->run(argc, argv);
	delete app;

	printf("Press any key to exit.\n");
	getchar();

	return 0;
}
