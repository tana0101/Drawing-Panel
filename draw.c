/******************************************************************
 * This program illustrates the fundamental instructions for handling
 * mouse and keyboeard events as well as menu buttons.
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>1

#include <GL/glut.h>

#define    SIZEX   2000
#define    SIZEY   2000

#define    HEIGHT  512 // 預設windows大小
#define    WIDTH   512 // 預設windows大小

#define    MY_QUIT -1
#define    MY_CLEAR -2
#define    MY_SAVE  -3
#define    MY_BLEND -4
#define    MY_LOAD  -5

// file
#define    MY_SAVE_FILE -6 // SAVE_FILE
#define    MY_LOAD_FILE -7 // LOAD_FILE
#define    MY_BLEND_FILE -8   // LOAD_BLEND_FILE
#define    load_file_name "test2.bmp"
#define    save_file_name "out.bmp"
#define    header_name "header.bmp"

#define    WHITE   1
#define    RED     2
#define    GREEN   3
#define    BLUE    4

#define    POINT   1
#define    LINE    2
#define    POLYGON 3
#define    CIRCLE  4
#define    CURVE   5
#define    STRING  6 //
#define    GRID_LINES 7//

// save and load
#define SAVE_NOW_IMAGE 0
#define LOAD_NOW_IMAGE 0
#define SAVE_GO_BACK 1
#define LOAD_GO_BACK 1
#define SAVE_VERSION 2
#define LOAD_VERSION 2
#define NUMBER_OF_VERSIONS 100 // 可回復的版本次數

// keyboard
#define CTRL_Z 26
#define CTRL_C 3
#define CTRL_X 24
#define CTRL_V 22
#define CTRL_B 2
#define ENTER 13
#define DELETE 8

// string
#define STR_INIT_POS_X HEIGHT/2
#define STR_INIT_POS_Y WIDTH/2
#define STR_HEIGHT 20
#define STR_WIDTH 20

typedef    int   menu_t;
menu_t     top_m, color_m, file_m, type_m;

int        height = HEIGHT, width = WIDTH;//
unsigned char  image[NUMBER_OF_VERSIONS][SIZEX * SIZEY][4];  /* Image data in main memory */

int        pos_x = -1, pos_y = -1;
int        ps_pos_x = -1, ps_pos_y = -1; //passivemotion
float      myColor[3] = { 0.0,0.0,0.0 };
int        obj_type = -1;
int        first = 0;      /* flag of initial points for lines and curve,..*/
int        vertex[128][2]; /*coords of vertices */
int        side = 0;         /*num of sides of polygon */
float      pnt_size = 1.0;
int        str_pos_x[NUMBER_OF_VERSIONS];//
int        str_pos_y[NUMBER_OF_VERSIONS];//

// 持久化 version
int version = 0; // 當前版本
int go_back_version; // 儲存欲回復的版本
int first_click = 0; // 第一次進入功能狀態時，要將跟隨滑鼠的點清掉，並且要做適當的儲存。
#define next_version (version + 1) % NUMBER_OF_VERSIONS
#define last_version (version + NUMBER_OF_VERSIONS - 1) % NUMBER_OF_VERSIONS

/*---------------------------------------------------------
 * 存下當前畫面
 */
void save(int operator)
{
    int i, j;
    
    if (operator == SAVE_NOW_IMAGE || operator == SAVE_VERSION) {
        if (operator == SAVE_NOW_IMAGE) version = next_version;
        glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE,
            image[version]); //第一個向素的座標/長寬矩形/格式/類型/返回向素資訊的陣列
        // 設定透明度
        for (i = 0; i < width; i++)   /* Assign 0 opacity to black pixels */
            for (j = 0; j < height; j++)
                if (image[version][i * width + j][0] == 0 &&
                    image[version][i * width + j][1] == 0 &&
                    image[version][i * width + j][2] == 0) image[version][i * width + j][3] = 0;
                else image[version][i * width + j][3] = 127; /* Other pixels have A=127*/
        
        // 更新string版本
        str_pos_x[version] = str_pos_x[last_version];
        str_pos_y[version] = str_pos_y[last_version];
 
        if (operator == SAVE_VERSION) {
            if (obj_type == STRING) { // 貼上版本後，要讓文字在下一格寫入文字
                // 所以需要先處理下一格的位置
                if (str_pos_x[version] + STR_WIDTH > width) {
                    str_pos_y[version] = str_pos_y[last_version] - STR_HEIGHT;
                    str_pos_x[version] = STR_WIDTH / 2;
                }
                else str_pos_x[version] = str_pos_x[last_version] + STR_WIDTH;
            }
            go_back_version = version;
        }
    }
    glFlush();
}

/*---------------------------------------------------------
 * 重新繪製畫面
 */
void load(int operator)
{   
    if (operator == LOAD_GO_BACK) { // 回到上一步
        version = last_version;
    }
    else if (operator== LOAD_VERSION) { // 回到特定版本
        version = go_back_version;
    }
    glRasterPos2i(0, 0); //設定繪圖的起始繪製
    glDrawPixels(width, height,
        GL_RGBA, GL_UNSIGNED_BYTE,
        image[version]); // 將image內的資料繪製
    glFlush();
}

/*------------------------------------------------------------
 * Callback function for display, redisplay, expose events
 * Just clear the window again
 */
void display_func(void)
{
    /* define window background color */
    save(SAVE_NOW_IMAGE); //存下當前畫面
    glClear(GL_COLOR_BUFFER_BIT); // 清除色彩緩衝區。
    load(LOAD_NOW_IMAGE); //重新繪製畫面
    glFlush(); //清除緩存區，讓命令提前執行完成。
}

/*------------------------------------------------------------
 * 檔案處理
 */
GLint imagewidth;
GLint imageheight;
GLint imagesize;
GLubyte* loadimage;

FILE* pfile;
FILE* pwrite;
FILE* pheader;
GLubyte header[54];

int image_temp[SIZEX * SIZEY][4]; // 暫存目前畫布，方便RGBA跟BGR的混和

void blend(int operator) {

    int i, j;

    if (operator == MY_BLEND) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // RGBA混合因子
        glRasterPos2i(0, 0);
        glDrawPixels(width, height,
            GL_RGBA, GL_UNSIGNED_BYTE,
            image[go_back_version]); //RGBA存入
        glDisable(GL_BLEND);
        display_func();
    }
    else if (operator == MY_BLEND_FILE) {
        glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE,
            image_temp); // 將目前畫布暫存
        glDrawPixels(imagewidth, imageheight,
            GL_BGR_EXT, GL_UNSIGNED_BYTE,
            loadimage); // 將file讀入
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // RGBA混合因子
        glRasterPos2i(0, 0);
        glDrawPixels(width, height,
            GL_RGBA, GL_UNSIGNED_BYTE,
            image_temp); // 混合
        glDisable(GL_BLEND);
    }

    glFlush();
}

void load_file(int operator) {
    // open image
    fopen_s(&pfile, load_file_name, "rb"); // 欲載入的file
    if (pfile == 0) exit(0);

    // image_size
    fseek(pfile, 0x0012, SEEK_SET); // 點陣圖偏移量：寬度(12h)，高度(16h)
    fread(&imagewidth, sizeof(imagewidth), 1, pfile);
    fread(&imageheight, sizeof(imageheight), 1, pfile);
    imagesize = imagewidth * imageheight * 3; // RGB

    // load image data
    loadimage = (GLubyte*)malloc(imagesize);
    if (loadimage == 0) exit(0);
    fseek(pfile, 54, SEEK_SET); // 前54是標頭檔長度
    fread(loadimage, imagesize, 1, pfile);

    // draw pixels
    if (operator == MY_LOAD_FILE) {
        glDrawPixels(imagewidth, imageheight,
            GL_BGR_EXT, GL_UNSIGNED_BYTE,
            loadimage); // 將image內的資料繪製(使用GL_BGR_EXT)
    }
    else if (operator == MY_BLEND_FILE) {
        blend(MY_BLEND_FILE);
    }

    //close file
    fclose(pfile);
    display_func();
}

void save_file() {
    // open file
    fopen_s(&pheader, header_name, "rb"); // 預先寫好的header
    if (pheader == 0) exit(0);
    fopen_s(&pwrite, save_file_name, "wb"); // 欲寫入的file
    if (pwrite == 0) exit(0);

    // load image data
    glReadPixels(0, 0, width, height,
        GL_BGR_EXT, GL_UNSIGNED_BYTE, image); // 將image內的資料載入(使用GL_BGR_EXT)

    // write header
    fread(header, sizeof(header), 1, pheader); // 載入預先寫好的header
    fwrite(header, sizeof(header), 1, pwrite); // 寫入header

    // write image width and height
    fseek(pwrite, 0x0012, SEEK_SET); // 點陣圖偏移量：寬度(12h)，高度(16h)
    fwrite(&width, sizeof(width), 1, pwrite);
    fwrite(&height, sizeof(height), 1, pwrite);

    // write image data
    imagesize = width * height * 3; // RGB
    fseek(pwrite, 54, SEEK_SET); // 從標頭檔後開始寫入image
    fwrite(image, imagesize, 1, pwrite);

    //close file
    fclose(pwrite);
    fclose(pheader);
}


/*-------------------------------------------------------------
 * reshape callback function for window.
 */
void my_reshape(int new_w, int new_h)
{
    height = new_h;
    width = new_w;

    glMatrixMode(GL_PROJECTION); // 投影模式
    glLoadIdentity(); // 將矩陣設為單位矩陣
    gluOrtho2D(0.0, (double)width, 0.0, (double)height); // 透過正交投影，定義出裁剪的面
    glViewport(0, 0, width, height); // 可視視窗，將螢幕的位置對應到模型的範圍內，才看得到繪製的東西。
    glMatrixMode(GL_MODELVIEW); // 模型視窗模式

    glutPostRedisplay();   /*---Trigger Display event for redisplay window*/
}


/*---------------------------------------------------------
 * Procedure to draw a polygon
 */
void draw_polygon()
{
    int  i;

    glBegin(GL_POLYGON);
    for (i = 0; i < side; i++)
        glVertex2f(vertex[i][0], height - vertex[i][1]);
    glEnd();
    glFinish();
    side = 0;    /* set side=0 for next polygon */
}



/*------------------------------------------------------------
 * Procedure to draw a circle
 */
void draw_circle()
{
    static GLUquadricObj* mycircle = NULL;

    if (mycircle == NULL) {
        mycircle = gluNewQuadric(); // 建立四邊形物件
        gluQuadricDrawStyle(mycircle, GLU_FILL);
    }
    glPushMatrix();
    glTranslatef(pos_x, height - pos_y, 0.0);
    gluDisk(mycircle,
        0.0,           /* inner radius=0.0 */
        10.0,          /* outer radius=10.0 */
        16,            /* 16-side polygon */
        3);
    glPopMatrix();
}



/*------------------------------------------------------------
 * Procedure to draw a string
 */
void draw_string(const char* string)
{
    glRasterPos2f(str_pos_x[version], str_pos_y[version]);
    glutBitmapString(GLUT_BITMAP_TIMES_ROMAN_24, string);
    glFinish();
    display_func();
}

void string_endl() // 換行
{
    str_pos_y[version] = str_pos_y[last_version]-STR_HEIGHT;
    str_pos_x[version] = STR_WIDTH / 2;
}

void string_init() // 初始化位置
{
    str_pos_x[version] = STR_INIT_POS_X;//
    str_pos_y[version] = STR_INIT_POS_Y;//
}



/*------------------------------------------------------------
 * Procedure to draw a grid line
 */
void draw_grid() {

    int i, j;

    for (i = 0; i < width; i+=64){
        for (j = 0; j < height; j+=64) {
            glLineWidth(pnt_size);    
            glBegin(GL_LINES);
            glVertex2f(i, j);
            glVertex2f(width, j);
            glVertex2f(i, j);
            glVertex2f(i, height);
            glEnd();
        }
    }
    glFinish();
    save(SAVE_NOW_IMAGE);
}

/*-------------------------------------------------------------
 * passivemotion to draw point
 */
void draw_passivemotion_point() {
    static GLUquadricObj* mycircle = NULL;

    if (mycircle == NULL) {
        mycircle = gluNewQuadric(); // 建立四邊形物件
        gluQuadricDrawStyle(mycircle, GLU_FILL);
    }
    glPushMatrix();
    glTranslatef(ps_pos_x, height - ps_pos_y, 0.0);
    gluDisk(mycircle,
        0.0,           /* inner radius=0.0 */
        5.0,          /* outer radius=10.0 */
        16,            /* 16-side polygon */
        3);
    glPopMatrix();
    glFinish();
}

/*------------------------------------------------------------
 * Callback function handling mouse-press events
 */
void mouse_func(int button, int state, int x, int y)
{   
    if (first_click == 1) {
        first_click= 0 ;
        load(LOAD_NOW_IMAGE);
    }
    if (button != GLUT_LEFT_BUTTON || state != GLUT_DOWN)
        return;
    switch (obj_type) {
    case POINT:
        glPointSize(pnt_size);     /*  Define point size */
        glBegin(GL_POINTS);     /*  Draw a point */
        glVertex2f(x, height - y);
        glEnd();
        break;
    case LINE:
        if (first == 0) {
            first = 1;
            pos_x = x; pos_y = y;
            glPointSize(pnt_size);
            glBegin(GL_POINTS);   /*  Draw the 1st point */
            glVertex3f(x, height - y, 0);
            glEnd();
        }
        else {
            first = 0;
            glLineWidth(pnt_size);     /* Define line width */
            glBegin(GL_LINES);    /* Draw the line */
            glVertex2f(pos_x, height - pos_y);
            glVertex2f(x, height - y);
            glEnd();
        }
        break;
    case POLYGON:  /* Define vertices of poly */
        if (side == 0) {
            vertex[side][0] = x; vertex[side][1] = y;
            side++;
        }
        else {
            if (fabs(vertex[side - 1][0] - x) + fabs(vertex[side - 1][1] - y) < 2)
                draw_polygon();
            else {
                glBegin(GL_LINES);
                glVertex2f(vertex[side - 1][0], height - vertex[side - 1][1]);
                glVertex2f(x, height - y);
                glEnd();
                vertex[side][0] = x;
                vertex[side][1] = y;
                side++;
            }
        }
        break;
    case CIRCLE:
        pos_x = x; pos_y = y;
        draw_circle();
        break;
    default:
        break;
    }
    glFinish();
}



/*-------------------------------------------------------------
 * motion callback function. The mouse is pressed and moved.
 */
void motion_func(int  x, int y)
{
    if (obj_type != CURVE) return;
    if (first == 0) {
        first = 1;
        pos_x = x; pos_y = y;
    }
    else {
        glBegin(GL_LINES);
        glVertex3f(pos_x, height - pos_y, 0.0);
        glVertex3f(x, height - y, 0.0);
        glEnd();
        pos_x = x; pos_y = y;
    }
    glFinish();
}
/*-------------------------------------------------------------
 * passivemotion callback function. The mouse is moved
 */
void passivemotion_func(int x, int y) {
    save(SAVE_NOW_IMAGE);
    load(LOAD_GO_BACK);
    ps_pos_x = x;
    ps_pos_y = y;
    draw_passivemotion_point();
    first_click = 1;
}

/*--------------------------------------------------------
 * procedure to clear window
 */
void init_window(void)
{
    
    /*Do nothing else but clear window to black*/

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, (double)width, 0.0, (double)height);
    glViewport(0, 0, width, height);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glClearColor(0.0, 0.0, 0.0, 0.0); //清空的顏色
    glClear(GL_COLOR_BUFFER_BIT);
    glFlush();
    save(LOAD_NOW_IMAGE);
}

/*--------------------------------------------------------------
 * Callback function for keyboard event.
 * key = the key pressed,
 * (x,y)= position in the window, where the key is pressed.
 */
void keyboard(unsigned char key, int x, int y)
{   
    if (first_click == 1) {
        first_click = 0;
        load(LOAD_NOW_IMAGE);
    }
    fprintf(stderr, "%c %d\n",key,(int)(key));
    if (obj_type != STRING && (key == 'Q' || key == 'q')) exit(0);
    else if ((int)(key) == CTRL_Z) { // 回復上一個版本
        load(LOAD_GO_BACK);
    }
    else if ((int)(key) == CTRL_C) { // 複製當前版本
        save(SAVE_VERSION);
    }
    else if ((int)(key) == CTRL_X) { // 剪下當前版本
        save(SAVE_VERSION);
        init_window();
    }
    else if ((int)(key) == CTRL_V) { // 貼上過去版本
        init_window();
        load(LOAD_VERSION);
    }
    else if ((int)key == CTRL_B) {
        blend(MY_BLEND);
    }
    else if (obj_type == STRING) { // 輸入文字
        if ((int)(key) == ENTER || str_pos_x[version] + STR_WIDTH > width) string_endl();
        else if ((int)(key) == DELETE) {
            load(LOAD_GO_BACK);
        }
        else {
            draw_string(&key);
            str_pos_x[version] = str_pos_x[last_version] + STR_WIDTH;
        }
    }
}


/*------------------------------------------------------
 * Procedure to initialize data alighment and other stuff
 */
void init_func()
{
    glReadBuffer(GL_FRONT);
    glDrawBuffer(GL_FRONT);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
}

/*-----------------------------------------------------------------
 * Callback function for color menu
 */
void  color_func(int value)
{
    if (first_click == 1) {
        first_click = 0;
        load(LOAD_NOW_IMAGE);
    }
    switch (value) {
    case WHITE:
        myColor[0] = myColor[1] = myColor[2] = 1.0;
        break;

    case RED:
        myColor[0] = 1.0;
        myColor[1] = myColor[2] = 0.0;
        break;

    case GREEN:
        myColor[0] = myColor[2] = 0.0;
        myColor[1] = 1.0;
        break;

    case BLUE:
        myColor[0] = myColor[1] = 0.0;
        myColor[2] = 1.0;
        break;

    default:
        break;
    }
    glColor3f(myColor[0], myColor[1], myColor[2]);
}

/*------------------------------------------------------------
 * Callback function for top menu.
 */
void file_func(int value)
{
    if (first_click == 1) {
        first_click = 0;
        load(LOAD_NOW_IMAGE);
    }
    int i, j;

    if (value == MY_QUIT) exit(0);
    else if (value == MY_CLEAR) init_window();
    else if (value == MY_SAVE) save(SAVE_VERSION); /* Save current window*/
    else if (value == MY_LOAD) { /* Restore the saved image*/
        init_window();
        load(LOAD_VERSION);
    }
    else if (value == MY_SAVE_FILE) { /* Save current window to drive*/
        save_file();
    }
    else if (value == MY_LOAD_FILE) { /* Restore the saved image from drive*/
        load_file(MY_LOAD_FILE);
    }
    else if (value == MY_BLEND) { /* Blending current image with the saved image */
        blend(MY_BLEND);
    }
    else if (value == MY_BLEND_FILE) { // 讀入圖檔並混和目前畫布
        load_file(MY_BLEND_FILE);
    }

    glFlush();
}

void size_func(int value)
{
    if (value == 1) {
        if (pnt_size < 10.0) pnt_size += 1.0;
    }
    else {
        if (pnt_size > 1.0) pnt_size = pnt_size - 1.0;
    }
}

/*---------------------------------------------------------------
 * Callback function for top menu. Do nothing.
 */
void top_menu_func(int value)
{
}


/*-------------------------------------------------------------
 * Callback Func for type_m, define drawing object
 */
void draw_type(int value)
{
    if (first_click == 1) {
        first_click = 0;
        load(LOAD_NOW_IMAGE);
    }
    obj_type = value;
    if (value == LINE || value == CURVE)
        first = 0;
    else if (value == POLYGON) side = 0;
    else if (value == STRING) string_init();
    else if (value == GRID_LINES) draw_grid();
}

void func() {
    ps_pos_x = pos_x;
    ps_pos_y = pos_y;
    draw_passivemotion_point();
}

/*---------------------------------------------------------------
 * Main procedure sets up the window environment.
 */
void main(int argc, char** argv)
{
    int  size_menu;

    glutInit(&argc, argv);    /*---Make connection with server---*/

    glutInitWindowPosition(0, 0);  /*---Specify window position ---*/
    glutInitWindowSize(width, height); /*--Define window's height and width--*/

    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA); /*---set display mode---*/
    init_func();

    /* Create parent window */
    glutCreateWindow("Menu");

    glutDisplayFunc(display_func); /* Associate display event callback func */
    glutReshapeFunc(my_reshape);  /* Associate reshape event callback func */

    glutKeyboardFunc(keyboard); /* Callback func for keyboard event */
    //glutIdleFunc(func);
    glutMouseFunc(mouse_func);  /* Mouse Button Callback func */
    glutMotionFunc(motion_func);/* Mouse motion event callback func */
    glutPassiveMotionFunc(passivemotion_func);
    

    color_m = glutCreateMenu(color_func); /* Create color-menu */
    glutAddMenuEntry("white", WHITE);
    glutAddMenuEntry("red", RED);
    glutAddMenuEntry("green", GREEN);
    glutAddMenuEntry("blue", BLUE);

    file_m = glutCreateMenu(file_func);   /* Create another menu, file-menu */
    glutAddMenuEntry("save", MY_SAVE);
    glutAddMenuEntry("load", MY_LOAD);
    glutAddMenuEntry("blend", MY_BLEND);
    glutAddMenuEntry("clear", MY_CLEAR);
    glutAddMenuEntry("quit", MY_QUIT);
    // 檔案處理
    glutAddMenuEntry("save file", MY_SAVE_FILE); 
    glutAddMenuEntry("load file", MY_LOAD_FILE);
    glutAddMenuEntry("blend file", MY_BLEND_FILE);

    type_m = glutCreateMenu(draw_type);   /* Create draw-type menu */
    glutAddMenuEntry("Point", POINT);
    glutAddMenuEntry("Line", LINE);
    glutAddMenuEntry("Poly", POLYGON);
    glutAddMenuEntry("Curve", CURVE);
    glutAddMenuEntry("Circle", CIRCLE);
    glutAddMenuEntry("String", STRING);
    glutAddMenuEntry("Grid line", GRID_LINES);

    size_menu = glutCreateMenu(size_func);
    glutAddMenuEntry("Bigger", 1);
    glutAddMenuEntry("Smaller", 2);

    top_m = glutCreateMenu(top_menu_func);/* Create top menu */
    glutAddSubMenu("colors", color_m);    /* add color-menu as a sub-menu */
    glutAddSubMenu("type", type_m);
    glutAddSubMenu("Size", size_menu);
    glutAddSubMenu("file", file_m);       /* add file-menu as a sub-menu */
    glutAttachMenu(GLUT_RIGHT_BUTTON);    /* associate top-menu with right but*/

    /*---Test whether overlay support is available --*/
    if (glutLayerGet(GLUT_OVERLAY_POSSIBLE)) {
        fprintf(stderr, "Overlay is available\n");
    }
    else {
        fprintf(stderr, "Overlay is NOT available, May encounter problems for menu\n");
    }
    /*---Enter the event loop ----*/
    glutMainLoop();
}