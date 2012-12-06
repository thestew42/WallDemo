/*----------------------------------------------------------------------------*\
|The main class for a node. Listens for a connection from the host application |
|and opens a window when connection is established. Receives and forwards      |
|OpenGL commands from the host.                                                |
|                                                                              |
|Depends on Ws2_32.lib                                                         |
|                                                                              |
|Stewart Hall                                                                  |
|11/19/2012                                                                    |
\*----------------------------------------------------------------------------*/

#include "GLNode.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

//Array used to keep track of keyboard input
bool	keys[256];

//Window active flag
bool	active=TRUE;

//OpenGL function handlers
typedef void (GLNode::*GLFHandler)();
GLFHandler handlers[] = {NULL,
	&GLNode::_glClearColor,
	&GLNode::_glClear,
	&GLNode::_glLoadIdentity,
	&GLNode::_glTranslatef,
	&GLNode::_glBegin,
	&GLNode::_glEnd,
	&GLNode::_glVertex3f,
	&GLNode::_glColor3f,
	&GLNode::_glRotatef,
	&GLNode::_glScalef
};

//Constructor for the GLNode
GLNode::GLNode(char *i_configFile, char *i_nodeIndentifier)
{
	configFile = i_configFile;
	nodeIdentifier = i_nodeIndentifier;

	hDC = NULL;
	hRC = NULL;
	associated_hRC = NULL;
	hWnd = NULL;
	fullscreen = 0;
	multiGPU = 0;
	sync = FALSE;

	buffer = new char[BUFFER_SIZE];
	buffer_pointer = 0;

	//Read configuration from file into members
	readConfiguration();
}

//Destructor
GLNode::~GLNode()
{
	delete buffer;
}

//Reads data from the configuration file into this node's properties
void GLNode::readConfiguration()
{
	char *line = new char[256];
	char *tag;
	char *number;
	FILE *fp = fopen(configFile, "r");

	if(fp) {
		//Read each line in the config file sequentially
		while(!feof(fp)) {
			//Read a line
			fgets(line, 256, fp);

			//Extract the tag on this line
			tag = strtok(line, " :");
			if(!tag)
				continue;

			//Look for global variables or this node's sub-section
			if(!strcmp(tag, "totalWidth")) {
				number = strtok(NULL, " :");
				sscanf(number, "%d", &host_width);
			} else if(!strcmp(tag, "totalHeight")) {
				number = strtok(NULL, " :");
				sscanf(number, "%d", &host_height);
			} else if(!strcmp(tag, "nodes")) {
				//Ignore
			} else if(!strcmp(tag, "multiGPU")) {
				number = strtok(NULL, " :");
				sscanf(number, "%d", &multiGPU);
			} else if(!strcmp(tag, nodeIdentifier)) {
				//Read this node's properties
				while(!feof(fp)) {
					//Read a line
					if(!fgets(line, 256, fp))
						break;

					//Extract the tag on this line
					tag = strtok(line, " :");
					if(!tag)
						break;

					if(!strcmp(tag, "width")) {
						number = strtok(NULL, " :");
						sscanf(number, "%d", &win_width);
					} else if(!strcmp(tag, "height")) {
						number = strtok(NULL, " :");
						sscanf(number, "%d", &win_height);
					} else if(!strcmp(tag, "xOffset")) {
						number = strtok(NULL, " :");
						sscanf(number, "%d", &x_offset);
					} else if(!strcmp(tag, "yOffset")) {
						number = strtok(NULL, " :");
						sscanf(number, "%d", &y_offset);
					} else if(!strcmp(tag, "xLocation")) {
						number = strtok(NULL, " :");
						sscanf(number, "%d", &x_location);
					} else if(!strcmp(tag, "yLocation")) {
						number = strtok(NULL, " :");
						sscanf(number, "%d", &y_location);
					} else if(!strcmp(tag, "fullscreen")) {
						number = strtok(NULL, " :");
						sscanf(number, "%d", &fullscreen);
					} else if(!strcmp(tag, "device")) {
						number = strtok(NULL, " :");
						sscanf(number, "%d", &device_id);
					} else if(!strcmp(tag, "address")) {
						number = strtok(NULL, " :");
						sscanf(number, "%d", &port);
					} else {
						break;
					}
				}
			}
		}
		fclose(fp);
	} else {
		printf("Error opening config file\n");
	}

	delete line;
}

//Print the configuration data and connection status for this node
void GLNode::printStatus()
{
	printf("GLNode configuration and status:\n");
	printf("\tHost application dimensions: %dpx x %dpx\n", host_width, host_height);
	printf("\tWindow dimensions: %dpx x %dpx\n", win_width, win_height);
	printf("\tWindow offset: %dpx, %dpx\n", x_offset, y_offset);
	printf("\tWindow location: %dpx, %dpx\n", x_location, y_location);
	printf("\tFullscreen: %d\n", fullscreen);
	printf("\tUse graphics device: %d\n", device_id);
	printf("\tListen on port: %d\n", port);
}

//Sets up socket to listen and waits for connections
void GLNode::startListening()
{
	//Start winsock
	WSADATA wsaData;
	int starterr = WSAStartup(MAKEWORD(2,2), &wsaData);

	//Error checking
	if(starterr != 0) {
		printf("Error %d occurred!\n",  WSAGetLastError());
		WSACleanup();
		return;
	}
	
	//Create the socket
	SOCKET server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(server_sock == INVALID_SOCKET) {
		printf("Error %d occurred!\n",  WSAGetLastError());
		WSACleanup();
		return;
	}

	//Bind the socket to the port from the config file
	sockaddr_in anews;
	anews.sin_port = htons(port);
	anews.sin_addr.s_addr = INADDR_ANY;
	anews.sin_family = AF_INET;
	if(bind(server_sock, (sockaddr*)&anews, sizeof(anews)) == SOCKET_ERROR) {
		printf("Error %d occurred!\n",  WSAGetLastError());
		WSACleanup();
		return;
	}

	//Start listening for connections
	if(listen(server_sock, SOMAXCONN) == SOCKET_ERROR) {
		printf("Error %d occurred!\n",  WSAGetLastError());
		WSACleanup();
		return;
	}

	//Accept a connection
	node_sock = accept(server_sock, NULL, NULL);
	if(node_sock == INVALID_SOCKET) {
		printf("Error %d occurred!\n",  WSAGetLastError());
		WSACleanup();
		return;
	}

	//Start connection handling code
	acceptConnection();

	//Clean up and exit
	printf("Cleaning up and exiting from listen code\n");
	closesocket(node_sock);
	closesocket(server_sock);

	WSACleanup();
	return;
}

//Accepts a connection by setting up a window and starting the main loop
void GLNode::acceptConnection()
{
	char *buffer = new char[256];
	int length = 0;

	//Receive node name from host to make sure connection is valid
	if((length = recv(node_sock, buffer, 256, 0)) != 256) {
		if(length == 0)
			printf("Connection was terminated unexpectedly\n");
		else
			printf("Error %d occurred!\n",  WSAGetLastError());
		return;
	}

	if(strcmp(buffer, nodeIdentifier) != 0) {
		printf("Incorrect node identifier sent by the host. Terminating connection.\n");
		return;
	}

	//Create the OpenGL window
	if(createWindow(nodeIdentifier, win_width, win_height, 24)) {
		//Start looping and waiting for OpenGL commands from the host
		mainLoop();
	}

	//Kill the window
	KillGLWindow();

	delete buffer;

	return;
}

//Handles messages and relays commands from the host
void GLNode::mainLoop()
{
	MSG msg;

	//Create fd_set for checking socket status
	fd_set conn;
	FD_ZERO(&conn);
	FD_SET(node_sock, &conn);

	//Timeout for select operation
	timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

	//False while main loop still running
	done = FALSE;
	
	while(!done) {
		//Check if a message is ready
		if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			if(msg.message == WM_QUIT) {
				done = TRUE;
			} else {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		} else {
			if(keys[VK_ESCAPE])
				done=TRUE;
			
			if(sync) {
				sync = FALSE;

				//If associated context is used, blit to the visible context
				if(associated_hRC) {
					glBindFramebuffer(GL_DRAW_FRAMEBUFFER, nRemoteDataFBO);

					//Release semaphore and wait for blit to finish
					ReleaseSemaphore(hSemaphore, 1, NULL);

					WaitForSingleObject(hSemaphore2, INFINITE);

					//Break binding to render to screen
					glBindFramebuffer(GL_FRAMEBUFFER, 0);

					//Render buffer to screen using a fullscreen quad
					glMatrixMode(GL_PROJECTION);
					glLoadIdentity();
					glMatrixMode(GL_MODELVIEW);
					glLoadIdentity();
					glBindTexture(GL_TEXTURE_2D, textureId);
					glBegin(GL_QUADS);
					glTexCoord2f(0.0f, 0.0f);
					glVertex3f(-1.0f,-1.0f, -1.0f);
					glTexCoord2f(1.0f, 0.0f);
					glVertex3f( 1.0f,-1.0f, -1.0f);
					glTexCoord2f(1.0f, 1.0f);
					glVertex3f( 1.0f, 1.0f, -1.0f);
					glTexCoord2f(0.0f, 1.0f);
					glVertex3f(-1.0f, 1.0f, -1.0f);
					glEnd();
					glBindTexture(GL_TEXTURE_2D, 0);
				}

				SwapBuffers(hDC);
			}
		}

		//Check if there is a command waiting from the host and accept it
		if(!associated_hRC && select(0, &conn, NULL, NULL, &timeout) > 0)
			receiveCommand();
	}
}

//Receives an OpenGL command and runs it
int GLNode::receiveCommand()
{
	int id;
	int length;
	
	//Grab the command ID
	if((length = recv(node_sock, (char*)(&id), sizeof(int), 0)) != sizeof(int)) {
		if(length == 0)
			printf("Connection was terminated unexpectedly\n");
		else
			printf("Error %d occurred!\n",  WSAGetLastError());
		return -1;
	}

	if(id == 0) {
		//This is a syncronize message, signal the window to swap buffers
		sync = TRUE;
		sync2 = TRUE;
	} else {
		//Call the correct handler
		(this->*handlers[id])();
	}

	return 0;
}

//Prepares the internal buffer for arguments
void GLNode::prepareBuffer(unsigned int length)
{
	int recv_length;
	
	//Grab the command arguments
	if((recv_length = recv(node_sock, buffer, length, 0)) != length) {
		if(recv_length == 0)
			printf("Connection was terminated unexpectedly\n");
		else
			printf("Error %d occurred!\n",  WSAGetLastError());
		return;
	}

	//Reset pointer
	buffer_pointer = 0;
}

//Gets a GLfloat from the buffer
void GLNode::getGLfloat(GLfloat *value)
{
	memcpy(value, &buffer[buffer_pointer], sizeof(GLfloat));
	buffer_pointer += sizeof(GLfloat);
}

//Gets a GLbitfield from the buffer
void GLNode::getGLbitfield(GLbitfield *value)
{
	memcpy(value, &buffer[buffer_pointer], sizeof(GLbitfield));
	buffer_pointer += sizeof(GLbitfield);
}

//Gets a GLenum value from the buffer
void GLNode::getGLenum(GLenum *value)
{
	memcpy(value, &buffer[buffer_pointer], sizeof(GLenum));
	buffer_pointer += sizeof(GLenum);
}

/*  This Code Creates Our OpenGL Window.  Parameters Are:                   *
 *  title           - Title To Appear At The Top Of The Window              *
 *  width           - Width Of The GL Window Or Fullscreen Mode             *
 *  height          - Height Of The GL Window Or Fullscreen Mode            *
 *  bits            - Number Of Bits To Use For Color (8/16/24/32)          *
 *  fullscreenflag  - Use Fullscreen Mode (TRUE) Or Windowed Mode (FALSE)   *
 *                                                                          *
 *  Code taken from NeHe (http://nehe.gamedev.net/) Tutorial 2              */
BOOL GLNode::createWindow(char* title, int width, int height, int bits)
{
	GLuint      PixelFormat;            // Holds The Results After Searching For A Match
	WNDCLASS    wc;                     // Windows Class Structure
	DWORD       dwExStyle;              // Window Extended Style
	DWORD       dwStyle;                // Window Style
	RECT        WindowRect;             // Grabs Rectangle Upper Left / Lower Right Values
	WindowRect.left=(long)0;            // Set Left Value To 0
	WindowRect.right=(long)width;       // Set Right Value To Requested Width
	WindowRect.top=(long)0;             // Set Top Value To 0
	WindowRect.bottom=(long)height;     // Set Bottom Value To Requested Height

	hInstance           = GetModuleHandle(NULL);                // Grab An Instance For Our Window
	wc.style            = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;   // Redraw On Size, And Own DC For Window.
	wc.lpfnWndProc      = (WNDPROC) WndProc;                    // WndProc Handles Messages
	wc.cbClsExtra       = 0;                                    // No Extra Window Data
	wc.cbWndExtra       = 0;                                    // No Extra Window Data
	wc.hInstance        = hInstance;                            // Set The Instance
	wc.hIcon            = LoadIcon(NULL, IDI_WINLOGO);          // Load The Default Icon
	wc.hCursor          = LoadCursor(NULL, IDC_ARROW);          // Load The Arrow Pointer
	wc.hbrBackground    = NULL;                                 // No Background Required For GL
	wc.lpszMenuName     = NULL;                                 // We Don't Want A Menu
	wc.lpszClassName    = "OpenGL";                             // Set The Class Name

	if (!RegisterClass(&wc))                                    // Attempt To Register The Window Class
	{
		MessageBox(NULL,"Failed To Register The Window Class.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;                                           // Return FALSE
	}
	
	/*if (fullscreen)                                             // Attempt Fullscreen Mode?
	{
		DEVMODE dmScreenSettings;                               // Device Mode
		memset(&dmScreenSettings,0,sizeof(dmScreenSettings));   // Makes Sure Memory's Cleared
		dmScreenSettings.dmSize=sizeof(dmScreenSettings);       // Size Of The Devmode Structure
		dmScreenSettings.dmPelsWidth    = width;                // Selected Screen Width
		dmScreenSettings.dmPelsHeight   = height;               // Selected Screen Height
		dmScreenSettings.dmBitsPerPel   = bits;                 // Selected Bits Per Pixel
		dmScreenSettings.dmFields=DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;

		// Try To Set Selected Mode And Get Results.  NOTE: CDS_FULLSCREEN Gets Rid Of Start Bar.
		if (ChangeDisplaySettings(&dmScreenSettings,CDS_FULLSCREEN)!=DISP_CHANGE_SUCCESSFUL)
		{
			// If The Mode Fails, Offer Two Options.  Quit Or Use Windowed Mode.
			if (MessageBox(NULL,"The Requested Fullscreen Mode Is Not Supported By\nYour Video Card. Use Windowed Mode Instead?","NeHe GL",MB_YESNO|MB_ICONEXCLAMATION)==IDYES)
			{
				fullscreen=FALSE;       // Windowed Mode Selected.  Fullscreen = FALSE
			}
			else
			{
				// Pop Up A Message Box Letting User Know The Program Is Closing.
				MessageBox(NULL,"Program Will Now Close.","ERROR",MB_OK|MB_ICONSTOP);
				return FALSE;                                   // Return FALSE
			}
		}
	}*/

	if (fullscreen)                                             // Are We Still In Fullscreen Mode?
	{
		dwExStyle=WS_EX_APPWINDOW;                              // Window Extended Style
		dwStyle=WS_POPUP;                                       // Windows Style
		ShowCursor(FALSE);                                      // Hide Mouse Pointer
	}
	else
	{
		dwExStyle=WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;           // Window Extended Style
		dwStyle=WS_OVERLAPPEDWINDOW;                            // Windows Style
	}

	AdjustWindowRectEx(&WindowRect, dwStyle, FALSE, dwExStyle);     // Adjust Window To True Requested Size

	// Create The Window
	if (!(hWnd=CreateWindowEx(  dwExStyle,                          // Extended Style For The Window
								"OpenGL",                           // Class Name
								title,                              // Window Title
								dwStyle |                           // Defined Window Style
								WS_CLIPSIBLINGS |                   // Required Window Style
								WS_CLIPCHILDREN,                    // Required Window Style
								x_location, y_location,             // Window Position
								WindowRect.right-WindowRect.left,   // Calculate Window Width
								WindowRect.bottom-WindowRect.top,   // Calculate Window Height
								NULL,                               // No Parent Window
								NULL,                               // No Menu
								hInstance,                          // Instance
								NULL)))                             // Dont Pass Anything To WM_CREATE
	{
		KillGLWindow();                             // Reset The Display
		MessageBox(NULL,"Window Creation Error.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;                               // Return FALSE
	}

	static  PIXELFORMATDESCRIPTOR pfd=              // pfd Tells Windows How We Want Things To Be
	{
		sizeof(PIXELFORMATDESCRIPTOR),              // Size Of This Pixel Format Descriptor
		1,                                          // Version Number
		PFD_DRAW_TO_WINDOW |                        // Format Must Support Window
		PFD_SUPPORT_OPENGL |                        // Format Must Support OpenGL
		PFD_DOUBLEBUFFER,                           // Must Support Double Buffering
		PFD_TYPE_RGBA,                              // Request An RGBA Format
		bits,                                       // Select Our Color Depth
		0, 0, 0, 0, 0, 0,                           // Color Bits Ignored
		0,                                          // No Alpha Buffer
		0,                                          // Shift Bit Ignored
		0,                                          // No Accumulation Buffer
		0, 0, 0, 0,                                 // Accumulation Bits Ignored
		16,                                         // 16Bit Z-Buffer (Depth Buffer)  
		0,                                          // No Stencil Buffer
		0,                                          // No Auxiliary Buffer
		PFD_MAIN_PLANE,                             // Main Drawing Layer
		0,                                          // Reserved
		0, 0, 0                                     // Layer Masks Ignored
	};
	
	if (!(hDC=GetDC(hWnd)))                         // Did We Get A Device Context?
	{
		KillGLWindow();                             // Reset The Display
		MessageBox(NULL,"Can't Create A GL Device Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;                               // Return FALSE
	}

	if (!(PixelFormat=ChoosePixelFormat(hDC,&pfd))) // Did Windows Find A Matching Pixel Format?
	{
		KillGLWindow();                             // Reset The Display
		MessageBox(NULL,"Can't Find A Suitable PixelFormat.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;                               // Return FALSE
	}

	if(!SetPixelFormat(hDC,PixelFormat,&pfd))       // Are We Able To Set The Pixel Format?
	{
		KillGLWindow();                             // Reset The Display
		MessageBox(NULL,"Can't Set The PixelFormat.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;                               // Return FALSE
	}

	if (!(hRC=wglCreateContext(hDC)))               // Are We Able To Get A Rendering Context?
	{
		KillGLWindow();                             // Reset The Display
		MessageBox(NULL,"Can't Create A GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;                               // Return FALSE
	}

	if(!wglMakeCurrent(hDC,hRC))                    // Try To Activate The Rendering Context
	{
		KillGLWindow();                             // Reset The Display
		MessageBox(NULL,"Can't Activate The GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;                               // Return FALSE
	}

	ShowWindow(hWnd,SW_SHOW);                       // Show The Window
	SetForegroundWindow(hWnd);                      // Slightly Higher Priority
	SetFocus(hWnd);                                 // Sets Keyboard Focus To The Window
	ReSizeGLScene(width, height);                   // Set Up Our Perspective GL Screen

	if (!InitGL())                                  // Initialize Our Newly Created GL Window
	{
		KillGLWindow();                             // Reset The Display
		MessageBox(NULL,"Initialization Failed.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;                               // Return FALSE
	}

	//Clear projection matrix if associated context was created
	if(associated_hRC != NULL) {
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
	}

	return TRUE;                                    // Success
}

/*  Kills the OpenGL window                                                 *
 *  Code taken from NeHe (http://nehe.gamedev.net/) Tutorial 2              */
GLvoid GLNode::KillGLWindow(GLvoid)								// Properly Kill The Window
{
	if (fullscreen)										// Are We In Fullscreen Mode?
	{
		ChangeDisplaySettings(NULL,0);					// If So Switch Back To The Desktop
		ShowCursor(TRUE);								// Show Mouse Pointer
	}

	if (hRC)											// Do We Have A Rendering Context?
	{
		if (!wglMakeCurrent(NULL,NULL))					// Are We Able To Release The DC And RC Contexts?
		{
			MessageBox(NULL,"Release Of DC And RC Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		}

		if (!wglDeleteContext(hRC))						// Are We Able To Delete The RC?
		{
			MessageBox(NULL,"Release Rendering Context Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		}
		hRC=NULL;										// Set RC To NULL
	}

	if (hDC && !ReleaseDC(hWnd,hDC))					// Are We Able To Release The DC
	{
		MessageBox(NULL,"Release Device Context Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		hDC=NULL;										// Set DC To NULL
	}

	if (hWnd && !DestroyWindow(hWnd))					// Are We Able To Destroy The Window?
	{
		MessageBox(NULL,"Could Not Release hWnd.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		hWnd=NULL;										// Set hWnd To NULL
	}

	if (!UnregisterClass("OpenGL",hInstance))			// Are We Able To Unregister Class
	{
		MessageBox(NULL,"Could Not Unregister Class.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		hInstance=NULL;									// Set hInstance To NULL
	}
}

/*  Resizes the OpenGL canvas                                               *
 *  Code taken from NeHe (http://nehe.gamedev.net/) Tutorial 2              */
GLvoid GLNode::ReSizeGLScene(GLsizei width, GLsizei height)
{
	if (height==0)										// Prevent A Divide By Zero By
	{
		height=1;										// Making Height Equal One
	}

	glViewport(0,0,width,height);						// Reset The Current Viewport

	glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
	glLoadIdentity();									// Reset The Projection Matrix

	//Calculate the correct frustum for this portion of the host application
	float near_clip = 0.1f;
	float far_clip = 100.0f;
	float fov = 45.0f;
	float aspect = (float)host_width / (float)host_height;
	float full_width = 2.0f * tan(fov * PI / 180.0f) * near_clip;
	float full_height = full_width / aspect;

	float part_width = (float)win_width / (float)host_width;
	float part_height = (float)win_height / (float)host_height;
	float left = (float)x_offset / (float)host_width - 0.5f;
	float right = left + part_width;
	float top = -1.0f * ((float)y_offset / (float)host_height - 0.5f);
	float bottom = top - part_height;

	left *= full_width;
	right *= full_width;
	bottom *= full_height;
	top *= full_height;

	glFrustum(left, right, bottom, top, near_clip, far_clip);

	glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
	glLoadIdentity();									// Reset The Modelview Matrix
}

void GLNode::OffscreenThreadMain()
{
	//Set as the current context
	if(wglMakeAssociatedContextCurrentAMD(associated_hRC) == FALSE)
	{
		printf("Error: could not make associated context current\n");
		return;
	}

	printf("Successfully created and set AMD GPU associated context\n");

	//Set up buffer
	UINT offscreenFBO, offscreenRBO;
	GLuint offscreenTexture;

	glGenTextures(1, &offscreenTexture);
	glBindTexture(GL_TEXTURE_2D, offscreenTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); 
	glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE); 
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, win_width, win_height, 0, GL_BGRA, GL_UNSIGNED_BYTE, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenFramebuffers(1, &offscreenFBO);
	glGenRenderbuffers(1, &offscreenRBO );
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, offscreenFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, offscreenRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, win_width, win_height);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	//Attach the texture to FBO color attachment point
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureId, 0);

	//Attach depth buffer
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, offscreenRBO);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, offscreenFBO);

	//Setup scene size on offscreen context
	ReSizeGLScene(win_width, win_height);

	glShadeModel(GL_SMOOTH);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	//Create fd_set for checking socket status
	fd_set conn;
	FD_ZERO(&conn);
	FD_SET(node_sock, &conn);

	//Timeout for select operation
	timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

	sync2 = FALSE;

	//Loop while not exiting
	while(!done)
	{
		if(sync2) {
			sync2 = FALSE;

			//Wait for the semaphore
			WaitForSingleObject(hSemaphore, INFINITE);

			//Blit to on screen window
			wglBlitContextFramebufferAMD(hRC,
							0, 0, win_width, win_height,
							0, 0, win_width, win_height,
							GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);

			//Insert fence to wait for completion
			remoteFence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

			// Wait for blit to finish
			GLenum BlitStatus = glClientWaitSync(remoteFence, GL_SYNC_FLUSH_COMMANDS_BIT, 100000000);
			if(BlitStatus == GL_CONDITION_SATISFIED  || BlitStatus == GL_ALREADY_SIGNALED) {
				
			} else {
				printf("Warning: Blit fail\n");
			}
			
			// Indicate that blit is finished
			ReleaseSemaphore(hSemaphore2, 1, NULL);
			
			glDeleteSync(remoteFence);
		}

		if(select(0, &conn, NULL, NULL, &timeout) > 0)
			receiveCommand();
	}

	//Clean up context
	if(!wglMakeAssociatedContextCurrentAMD(NULL))
	{
		MessageBox(NULL,"Unable to detatch associated context.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
	}
	if(!wglDeleteAssociatedContextAMD(associated_hRC))
	{
		MessageBox(NULL,"Release associated rendering context failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
	}
	associated_hRC = NULL;
}

DWORD WINAPI OffscreenRenderShell(LPVOID param)
{
	((GLNode*)param)->OffscreenThreadMain();
	return 0;
}

/*Sets up OpenGL and attempts to select a GPU based on the config parameters*/
int GLNode::InitGL(GLvoid)
{
	UINT *ids = new UINT[6];
	char *name = new char[256];
	int num_gpus = 0;

	if(GLEW_OK != glewInit())
		return FALSE;

	//Try to choose video card with WGL_AMD_gpu_association
	if(multiGPU && WGLEW_AMD_gpu_association)
	{
		num_gpus = wglGetGPUIDsAMD(6, ids);
		printf("Found %d AMD GPUs\n", num_gpus);

		if(device_id >= 0 && device_id < num_gpus) {
			printf("Selected GPU has ID %d\n", ids[device_id]);

			//Determine type of selected GPU
			wglGetGPUInfoAMD(ids[device_id], WGL_GPU_RENDERER_STRING_AMD, GL_UNSIGNED_BYTE, 256, name);
			printf("Selected GPU is %s\n", name);

			//Create a new context for this GPU and make it current
			associated_hRC = wglCreateAssociatedContextAMD(ids[device_id]);
			if(associated_hRC == NULL)
			{
				printf("Error: could not create associated context for selected GPU\n");
				return FALSE;
			}

			//Create semaphore
			hSemaphore = CreateSemaphore(NULL, 0, 1, NULL);
			hSemaphore2 = CreateSemaphore(NULL, 0, 1, NULL);

			//Launch new thread for this offscreen render context
			CreateThread(NULL, 0, OffscreenRenderShell, this, 0, NULL);

			printf("Successfully created context and launched thread\n");

			//Set up texture for full screen quad
			glGenTextures(1, &textureId);
			glBindTexture(GL_TEXTURE_2D, textureId);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); 
			glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE); 
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, win_width, win_height, 0, GL_BGRA, GL_UNSIGNED_BYTE, 0);
			glBindTexture(GL_TEXTURE_2D, 0);

			//Set up frame buffer
			glGenFramebuffers(1, &nRemoteDataFBO);
			glGenRenderbuffers(1, &nRemoteDataRBO);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, nRemoteDataFBO);
			glBindRenderbuffer(GL_RENDERBUFFER, nRemoteDataRBO);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, win_width, win_height);
			glBindRenderbuffer(GL_RENDERBUFFER, 0);

			//Attach the texture to FBO color attachment point
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureId, 0);

			//Attach the render buffer to depth
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, nRemoteDataRBO);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		} else {
			printf("Warning: device ID in config file (%d) is too high. Only %d GPUs available, using default.\n", device_id);
		}
	} else {
		if(multiGPU)
			printf("Warning: AMD_gpu_association is not supported. GPU will not be selected.\n");
		else
			printf("Use of explicit multi-gpu is disabled.\n");
	}

	delete [] ids;
	delete [] name;
	
	glShadeModel(GL_SMOOTH);
	glClearColor(0.0f, 0.0f, 0.0f, 0.5f);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	glDepthFunc(GL_LEQUAL);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	return TRUE;
}

/*  Callback to handle windows messages                                     *
 *  Code taken from NeHe (http://nehe.gamedev.net/) Tutorial 2              */
LRESULT CALLBACK WndProc(	HWND	hWnd,			// Handle For This Window
							UINT	uMsg,			// Message For This Window
							WPARAM	wParam,			// Additional Message Information
							LPARAM	lParam)			// Additional Message Information
{
	switch (uMsg)									// Check For Windows Messages
	{
		case WM_ACTIVATE:							// Watch For Window Activate Message
		{
			if (!HIWORD(wParam))					// Check Minimization State
			{
				active=TRUE;						// Program Is Active
			}
			else
			{
				active=FALSE;						// Program Is No Longer Active
			}

			return 0;								// Return To The Message Loop
		}

		case WM_SYSCOMMAND:
		{
			switch (wParam)
			{
				case SC_SCREENSAVE:
				case SC_MONITORPOWER:
					return 0;
			}
			break;
		}

		case WM_CLOSE:								// Did We Receive A Close Message?
		{
			PostQuitMessage(0);						// Send A Quit Message
			return 0;								// Jump Back
		}

		case WM_KEYDOWN:							// Is A Key Being Held Down?
		{
			keys[wParam] = TRUE;					// If So, Mark It As TRUE
			return 0;								// Jump Back
		}

		case WM_KEYUP:								// Has A Key Been Released?
		{
			keys[wParam] = FALSE;					// If So, Mark It As FALSE
			return 0;								// Jump Back
		}

		//case WM_SIZE:								// Resize The OpenGL Window
		//{
		//	ReSizeGLScene(LOWORD(lParam),HIWORD(lParam));  // LoWord=Width, HiWord=Height
		//	return 0;								// Jump Back
		//}
	}

	// Pass All Unhandled Messages To DefWindowProc
	return DefWindowProc(hWnd,uMsg,wParam,lParam);
}

//------------------------------------------------------------------------------
//Implementations of OpenGL functions
//------------------------------------------------------------------------------
//1: glClearColor — specify clear values for the color buffers
void GLNode::_glClearColor()
{
	GLfloat red, green, blue, alpha;
	prepareBuffer(4 * sizeof(GLfloat));
	getGLfloat(&red);
	getGLfloat(&green);
	getGLfloat(&blue);
	getGLfloat(&alpha);
	glClearColor(red, green, blue, alpha);
}

//2: glClear — clear buffers to preset values
void GLNode::_glClear()
{
	GLbitfield mask;
	prepareBuffer(sizeof(GLbitfield));
	getGLbitfield(&mask);
	glClear(mask);
}

//3: glLoadIdentity — replace the current matrix with the identity matrix
void GLNode::_glLoadIdentity()
{
	glLoadIdentity();
}

//4: glTranslatef — multiply the current matrix by a translation matrix
void GLNode::_glTranslatef()
{
	GLfloat x, y, z;
	prepareBuffer(3 * sizeof(GLfloat));
	getGLfloat(&x);
	getGLfloat(&y);
	getGLfloat(&z);
	glTranslatef(x, y, z);
}

//5: glBegin — delimit the vertices of a primitive or a group of like primitives
void GLNode::_glBegin()
{
	GLenum mode;
	prepareBuffer(sizeof(GLenum));
	getGLenum(&mode);
	glBegin(mode);
}

//6: glEnd — delimit the vertices of a primitive or a group of like primitives
void GLNode::_glEnd()
{
	glEnd();
}

//7: glVertex3f — Specifies a vertex
void GLNode::_glVertex3f()
{
	GLfloat x, y, z;
	prepareBuffer(3 * sizeof(GLfloat));
	getGLfloat(&x);
	getGLfloat(&y);
	getGLfloat(&z);
	glVertex3f(x, y, z);
}

//8: glColor3f — Sets the current color
void GLNode::_glColor3f()
{
	GLfloat red, green, blue;
	prepareBuffer(3 * sizeof(GLfloat));
	getGLfloat(&red);
	getGLfloat(&green);
	getGLfloat(&blue);
	glColor3f(red, green, blue);
}

//9: glRotatef — multiply the current matrix by a rotation matrix
void GLNode::_glRotatef()
{
	GLfloat angle, x, y, z;
	prepareBuffer(4 * sizeof(GLfloat));
	getGLfloat(&angle);
	getGLfloat(&x);
	getGLfloat(&y);
	getGLfloat(&z);
	glRotatef(angle, x, y, z);
}

//10: glScalef - multiply the current matrix by a general scaling matrix
void GLNode::_glScalef()
{
	GLfloat x, y, z;
	prepareBuffer(3 * sizeof(GLfloat));
	getGLfloat(&x);
	getGLfloat(&y);
	getGLfloat(&z);
	glScalef(x, y, z);
}
