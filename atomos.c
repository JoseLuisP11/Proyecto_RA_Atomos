#include <GL/glut.h>
#include <AR/gsub.h>
#include <AR/video.h>
#include <AR/param.h>
#include <AR/ar.h>
#include <math.h>
#include <time.h>


static GLint increment = 1; /*Incremento rotacion por fotograma*/

static GLint angleX = 0; /*Angulo de rotacion eje X*/
static GLint angleY = 0; /*Angulo de rotacion eje Y*/
static GLint angleZ = 0; /*Angulo de rotacion eje Z*/

static GLint rotatingX = 0;         /*Rotacion eje X*/
static GLint rotatingY = 0;         /*Rotacion eje Y*/
static GLint rotatingZ = 0;         /*Rotacion eje Z*/
static GLint rotatingDirection = 1; /*Rotacion sentido eje*/

static GLfloat electrones[20] = {0.0};
static GLint nobjects = 0;

// ==== Definicion de estructuras ===================================
struct TObject
{
    int id;                  // Identificador del patron
    int visible;             // Es visible el objeto?
    double width;            // Ancho del patron
    double center[2];        // Centro del patron
    double patt_trans[3][4]; // Matriz asociada al patron
    void (*drawme)(int);     // Puntero a funcion drawme
    int timer;

};

struct TObject *objects = NULL;

void print_error(char *error)
{
    printf("%s\n", error);
    exit(0);
}

void update()
{

    /* Rotación de los electrones alrededor del átomo */
    for (int i = 0; i < 6; i++)
    {
        electrones[i] += 1.0; // Incremento de la rotación para cada electrón
    }

    
    for (int i = 0; i < nobjects; i++)
    {
        // if(objects[i].timer > 0 && (time(NULL) - objects[i].timer) > 3){
        //     printf("3 segundos.\n");
        // }

        if(objects[i].timer > 0){

            printf("%li segundos.\n", time(NULL) - objects[i].timer);
        }

    }

    /* Fuerza re-dibujado */
    glutPostRedisplay();
}

// Definir una función para convertir radianes a grados (opcional)
double radians_to_degrees(double radians) {
    return radians * (180.0 / 3.1415);
}

// ======== obtainZangle (obtiene la rotacion en el eje Z del objeto) ====================================================
void obtainZangle(int id)
{
    double v[3];
    float module=0.0;

    // Obtenemos el angulo de rotacion
    v[0] = objects[id].patt_trans[0][0]; 
    v[1] = objects[id].patt_trans[1][0]; 
    v[2] = objects[id].patt_trans[2][0]; 
    
    module = sqrt(pow(v[0],2)+pow(v[1],2)+pow(v[2],2));
    v[0] = v[0]/module;  v[1] = v[1]/module; v[2] = v[2]/module; 

    // Interpretación del ángulo
    // if (fabs(angleZ) < 10) {
    //     printf("La marca está en su posición original (0 grados).\n");
    // } else if (fabs(angleZ - 180) < 10) {
    //     printf("La marca está del revés (180 grados).\n");
    // } else if (fabs(angleZ - 90) < 10) {
    //     if (v[1] > 0) {
    //         printf("La marca está girada hacia la derecha (90 grados).\n");
    //     } else {
    //         printf("La marca está girada hacia la izquierda (90 grados).\n");
    //     }
    // } else {
    //     printf("La marca está en una orientación no clasificada.\n");
    // }

    angleZ = acos (v[0]) * 57.2958;   // Sexagesimales! * (180/PI)
}


// ======== rotation (rota los atomos) ====================================================
void rotation()
{
    if (rotatingX == 1)
    {
        angleX = (angleX + increment * rotatingDirection) % 360;
    }
    else if (rotatingY == 1)
    {
        angleY = (angleY + increment * rotatingDirection) % 360;
    }
    else if (rotatingZ == 1)
    {
        angleZ = (angleZ + increment * rotatingDirection) % 360;
    }
    glutPostRedisplay();
}

// ======== keyboardSpecial (controla el sentido de rotacion horario o antihorario)===============================
void keyboardSpecial(int key, int x, int y)
{
    if (key == GLUT_KEY_RIGHT)
    {
        rotatingDirection = 1;
        glutIdleFunc(rotation);
    }
    else if (key == GLUT_KEY_LEFT)
    {
        rotatingDirection = -1;
        glutIdleFunc(rotation);
    }
}

// ==== addObject (Anade objeto a la lista de objetos) ==============
void addObject(char *p, double w, double c[2], void (*drawme)(int))
{
    int pattid;

    if ((pattid = arLoadPatt(p)) < 0)
        print_error("Error en carga de patron\n");

    nobjects++;
    objects = (struct TObject *)
        realloc(objects, sizeof(struct TObject) * nobjects);

    objects[nobjects - 1].id = pattid;
    objects[nobjects - 1].width = w;
    objects[nobjects - 1].center[0] = c[0];
    objects[nobjects - 1].center[1] = c[1];
    objects[nobjects - 1].drawme = drawme;
}

// ==== draw****** (Dibujado especifico de cada objeto) =============
void drawOxigen(int nobject)
{
    double gl_para[16]; // Esta matriz 4x4 es la usada por OpenGL
    argConvGlpara(objects[nobject].patt_trans, gl_para);
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixd(gl_para);

    glTranslatef(0.0, 0.0, 60.0);

    // Renderiza un átomo rojo
    glColor3ub(255, 0, 0);
    glutWireSphere(10, 500, 500);

    // Renderiza sus electrones
    for (int i = 0; i < 6; i++)
    {
        glPushMatrix();
        glRotatef(electrones[i], 0.0, 0.0, 1.0); // Rotación del electrón alrededor del átomo
        glRotatef(i * 45.0, 0.0, 0.0, 1.0);      // Rotación para posicionar el electrón alrededor del átomo
        glTranslatef(10 * 2.0, 0.0, 0.0);        // Distancia del electrón al átomo

        // Renderiza el electrón
        glColor3ub(0, 0, 0);
        glutWireSphere(2, 500, 500);

        glPopMatrix();
    }
}

void drawHydrogen(int nobject)
{
    double gl_para[16]; // Esta matriz 4x4 es la usada por OpenGL
    argConvGlpara(objects[nobject].patt_trans, gl_para);
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixd(gl_para);

    glTranslatef(0.0, 0.0, 60.0);

    // Renderiza un átomo blanco
    glColor3ub(255, 255, 255);
    glutWireSphere(10, 500, 500);

    // Renderiza sus electrones
    for (int i = 0; i < 2; i++)
    {
        glPushMatrix();
        glRotatef(electrones[i], 0.0, 0.0, 1.0); // Rotación del electrón alrededor del átomo
        glRotatef(i * 45.0, 0.0, 0.0, 1.0);      // Rotación para posicionar el electrón alrededor del átomo
        glTranslatef(10 * 2.0, 0.0, 0.0);        // Distancia del electrón al átomo

        // Renderiza el electrón
        glColor3ub(0, 0, 0);
        glutWireSphere(2, 500, 500);

        glPopMatrix();
    }
}

void drawDioxygen(double y_axis)
{
    glTranslatef(0.0, y_axis, 0.0);

    glPushMatrix();
    glTranslatef(0.0, 5.0, 60.0);

    // Renderiza un átomo rojo
    glColor3ub(255, 0, 0);
    glutWireSphere(10, 500, 500);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0, -5.0, 60.0);

    // Renderiza un átomo rojo
    glColor3ub(255, 0, 0);
    glutWireSphere(10, 500, 500);
    glPopMatrix();
}

void drawOxidane(double y_axis)
{
    glTranslatef(0.0, y_axis, 0.0);

    glPushMatrix();
    glTranslatef(0.0, 0.0, 60.0);

    // Renderiza un átomo rojo
    glColor3ub(255, 0, 0);
    glutWireSphere(10, 500, 500);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-5.0, -5.0, 60.0);

    // Renderiza un átomo blanco
    glColor3ub(255, 255, 255);
    glutWireSphere(5, 500, 500);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(5.0, 5.0, 60.0);

    // Renderiza un átomo blanco
    glColor3ub(255, 255, 255);
    glutWireSphere(5, 500, 500);
    glPopMatrix();
}

// ======== cleanup =================================================
static void cleanup(void)
{ // Libera recursos al salir ...
    arVideoCapStop();
    arVideoClose();
    argCleanup();
    free(objects);
    exit(0);
}

// ======== keyboard ================================================
static void keyboard(unsigned char key, int x, int y)
{
    switch (key)
    {
    case 0x1B:
    case 'Q':
    case 'q':
        cleanup();
        break;
    case 'Z':
    case 'z':
        rotatingZ = !rotatingZ;
        break;
    }
}

// ======== draw ====================================================
void draw(void)
{
    double gl_para[16]; // Esta matriz 4x4 es la usada por OpenGL
    GLfloat light_position[] = {100.0, -200.0, 200.0, 0.0};
    int i;

    argDrawMode3D();              // Cambiamos el contexto a 3D
    argDraw3dCamera(0, 0);        // Y la vista de la camara a 3D
    glClear(GL_DEPTH_BUFFER_BIT); // Limpiamos buffer de profundidad
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // for (i = 0; i < nobjects; i++)
    // {
    //     if (objects[i].visible)
    //     { // Si el objeto es visible
    //         argConvGlpara(objects[i].patt_trans, gl_para);
    //         glMatrixMode(GL_MODELVIEW);
    //         glLoadMatrixd(gl_para); // Cargamos su matriz de transf.

    //         // glEnable(GL_LIGHTING);
    //         // glEnable(GL_LIGHT0);
    //         // glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    //         // objects[i].drawme(); // Llamamos a su función de dibujar
    //     }
    // }

    double dist01; // Distancia entre el objeto 0 y 1
    double m[3][4], m2[3][4];

    if (objects[0].visible && objects[1].visible)
    {
        argConvGlpara(objects[0].patt_trans, gl_para);
        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixd(gl_para);

        arUtilMatInv(objects[0].patt_trans, m);
        arUtilMatMul(m, objects[1].patt_trans, m2);
        dist01 = sqrt(pow(m2[0][3], 2) + pow(m2[1][3], 2) + pow(m2[2][3], 2));
        // printf("Distancia objects[0] y objects[1]= %G\n", dist01);
        obtainZangle(1);

        if (dist01 < 120)
        {
            drawOxidane(dist01 / 2);
            // drawDioxygen(dist01/2);
        }
        else
        {
            objects[0].drawme(0); // Llamamos a su función de dibujar
            objects[0].timer=time(NULL);
            objects[1].drawme(1); // Llamamos a su función de dibujar
            objects[1].timer=time(NULL);
        }
    }
    else if (objects[0].visible)
    {
        objects[0].drawme(0); // Llamamos a su función de dibujar
        objects[0].timer=time(NULL);
    }
    else if (objects[1].visible)
    {
        obtainZangle(1);
        objects[1].drawme(0); // Llamamos a su función de dibujar
        objects[1].timer=time(NULL);
    }

    if (rotatingZ)
    {
        update();
    }

    glDisable(GL_DEPTH_TEST);
}

// ======== init ====================================================
static void init(void)
{
    ARParam wparam, cparam;   // Parametros intrinsecos de la camara
    int xsize, ysize;         // Tamano del video de camara (pixels)
    double c[2] = {0.0, 0.0}; // Centro de patron (por defecto)

    // Abrimos dispositivo de video
    if (arVideoOpen("-dev=/dev/video2") < 0)
        exit(0);
    if (arVideoInqSize(&xsize, &ysize) < 0)
        exit(0);

    // Cargamos los parametros intrinsecos de la camara
    if (arParamLoad("data/camera_para.dat", 1, &wparam) < 0)
        print_error("Error en carga de parametros de camara\n");

    arParamChangeSize(&wparam, xsize, ysize, &cparam);
    arInitCparam(&cparam); // Inicializamos la camara con "cparam"  

    // Inicializamos la lista de objetos
    addObject("data/simple.patt", 85.0, c, drawOxigen);
    addObject("data/identic.patt", 85.0, c, drawHydrogen);

    argInit(&cparam, 1.0, 0, 0, 0, 0); // Abrimos la ventana
}

// ======== mainLoop ================================================
static void mainLoop(void)
{
    ARUint8 *dataPtr;
    ARMarkerInfo *marker_info;
    int marker_num, i, j, k;

    // Capturamos un frame de la camara de video
    if ((dataPtr = (ARUint8 *)arVideoGetImage()) == NULL)
    {
        // Si devuelve NULL es porque no hay un nuevo frame listo
        arUtilSleep(2);
        return; // Dormimos el hilo 2ms y salimos
    }

    argDrawMode2D();
    argDispImage(dataPtr, 0, 0); // Dibujamos lo que ve la camara

    // Detectamos la marca en el frame capturado (return -1 si error)
    if (arDetectMarker(dataPtr, 100, &marker_info, &marker_num) < 0)
    {
        cleanup();
        exit(0); // Si devolvio -1, salimos del programa!
    }

    arVideoCapNext(); // Frame pintado y analizado... A por otro!

    // Vemos donde detecta el patron con mayor fiabilidad
    for (i = 0; i < nobjects; i++)
    {
        for (j = 0, k = -1; j < marker_num; j++)
        {
            if (objects[i].id == marker_info[j].id)
            {
                if (k == -1)
                    k = j;
                else if (marker_info[k].cf < marker_info[j].cf)
                    k = j;
            }
        }

        if (k != -1)
        { // Si ha detectado el patron
            objects[i].visible = 1;
            arGetTransMat(&marker_info[k], objects[i].center,
                          objects[i].width, objects[i].patt_trans);
            // printf("El factor de confianza es %f.\n", marker_info[k].cf);
            // printf("El patron es %d.\n", marker_info[k].id);
        }
        else
        {   // El objeto no es visible
            objects[i].visible = 0;
        } 
    }

    draw();           // Dibujamos los objetos de la escena
    argSwapBuffers(); // Cambiamos el buffer con lo que tenga dibujado
}

// ======== Main ====================================================
int main(int argc, char **argv)
{
    glutInit(&argc, argv); // Creamos la ventana OpenGL con Glut
    init();                // Llamada a nuestra funcion de inicio

    arVideoCapStart();                     // Creamos un hilo para captura de video
    argMainLoop(NULL, keyboard, mainLoop); // Asociamos callbacks
    glutSpecialFunc(keyboardSpecial);
    return (0);
}
