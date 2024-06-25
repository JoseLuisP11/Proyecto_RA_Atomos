#include <GL/glut.h>
#include <AR/gsub.h>
#include <AR/video.h>
#include <AR/param.h>
#include <AR/ar.h>
#include <AR/arMulti.h>
#include <math.h>
#include <time.h>

// Posiciones de las marcas en el array de la funcion init
const int ATOMPAT1 = 0;
const int ATOMPAT2 = 1;
const int ROTPAT = 2;
const int MULTPAT = 3;

// Distancia entre el patron ATOMPAT1 y el ATOMPAT2
const double dist01Const = 180;

// Posiciones del eje z del patron ROTPAT
const int UP = 0;
const int LEFT = 1;
const int RIGHT = 2;
const int DOWN = 3;
const int UNKNOWN = 4;


const int selectionTime = 5; // Distancia entre el patron ATOMPAT1 y el ATOMPAT2
const int thresholdM = 10;  // Umbral auxiliar para evitar zarandeos excesivos a la hora de dibujar atomos compuestos
const int thresholdZAngle = 10;  // Umbral auxiliar para calcular más precisamente los cambios en el eje Z,

static GLint increment = 1; /*Incremento rotacion por fotograma*/

// Tamanyos esferas
static GLfloat heightZ = 60.0; /*Altura por defecto de los atomos en el eje Z*/
static GLfloat lessHeightZ = 50.0; /*Altura rebajada de los atomos en el eje Z*/
static GLfloat defSlices = 500.0; /*Slices por defecto*/
static GLfloat defStacks = 500.0; /*Stacks por defecto*/
static GLfloat radElectron = 4.0; /*Radio por defecto del electron*/
static GLfloat radAtom = 15.0; /*Radio por defecto del atomo basico*/
static GLfloat radComAtom = 15.0; /*Radio por defecto del atomo compuesto*/
static GLfloat distComAtom1 = 5.0; /*Traslacion para atomos compuestos eje Y*/
static GLfloat distComAtom2 = 7.5; /*Traslacion para atomos compuestos eje Y*/
static GLfloat distComAtom3 = 10.0; /*Traslacion para atomos compuestos eje Y*/


static GLint rotZ = 0;   /*Sentido rotacion eje Z; 0 para posicion original 0 grados, 1 izquierda, 2 derecha, 3 del reves, 4 no determinable*/
static GLint fixed = 0;  

static GLint rotatingDirection = 1; /*Rotacion sentido eje*/

static GLfloat electrones[20] = {0.0};
static GLint nobjects = 0;
static GLint nobjectsMulti = 0;

ARMultiMarkerInfoT  *mMarker; // Estructura global multimarca

// ==== Definicion de estructuras ===================================
struct TObject
{
    int id;                  // Identificador del patron
    int visible;             // ¿Es observable en la escena el objeto?
    double width;            // Ancho del patron
    double center[2];        // Centro del patron
    double patt_trans[3][4]; // Matriz asociada al patron
    void (*drawme)(int);     // Puntero a funcion drawme
    void (*drawmeM)(double, double); // Puntero a funcion drawme de multipatron
    int timer;               // Temporizador para controlar apariciones de marcas  
    int markerType;          // Tipo de marcador; 0 para simple, 1 para multipatron
};

struct TObject *objects = NULL;         // Lista de patrones
struct TObject *objectsMulti = NULL;    // Lista de objetos para el multipatron

// ======== print_error (termina la ejecución del programa devolviendo el error)===============================
void print_error(char *error)
{
    printf("%s\n", error);
    exit(0);
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
    }
}



void update()
{

    /* Rotación de los electrones alrededor del átomo */
    for (int i = 0; i < 6; i++)
    {
        electrones[i] += 1.0; // Incremento de la rotación para cada electrón
    }

    /* Fuerza re-dibujado */
    glutPostRedisplay();
}

// ======== obtainZangle (obtiene la rotacion en el eje Z del patron) ====================================================
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

    double zAngle = acos (v[0]) * 57.2958;   // Sexagesimales * (180/PI)

    // Interpretación del ángulo
    if (fabs(zAngle) < thresholdZAngle) {
        rotZ = UP;  // La marca está en su posición original
    } else if (fabs(zAngle - 90) < thresholdZAngle) {
        if (v[1] <= 0) {
            rotZ = LEFT; // La marca está girada hacia la izquierda
        } else {
            rotZ = RIGHT; // La marca está girada hacia la derecha
        }
    } else if (fabs(zAngle - 180) < thresholdZAngle) {
        rotZ = DOWN; // La marca está del revés (180 grados)
    } else {
        rotZ = UNKNOWN; // La marca está en una orientación no determinada.
    }
}

// ==== addObjectMulti (Anade objetos a la lista de objetos del multipatron) ==============
void addObjectMulti(void (*drawme)(double,  double))
{
    nobjectsMulti++;    // Se aumenta el numero de objetos de la lista de objetos del multipatron
    objectsMulti = (struct TObject *) realloc(objectsMulti, sizeof(struct TObject) * nobjectsMulti);
    objectsMulti[nobjectsMulti - 1].drawmeM = drawme;   // Se guarda la funcion de dibujado
}

// ==== addObject (Anade objeto a la lista de patrones) ==============
void addObject(char *p, double w, double c[2], int markerType, void (*drawme)(int))
{
    int pattid;

    // markerType == 0 si es un patron simple, markerType== 0 si es multipatron
    if (markerType == 0 && (pattid = arLoadPatt(p)) < 0)
        print_error("Error en carga de patron\n");
    else if (markerType == 1 && (mMarker = arMultiReadConfigFile(p)) == NULL ){
        print_error("Error en carga de multipatron\n");
    }

    nobjects++;
    objects = (struct TObject *)
        realloc(objects, sizeof(struct TObject) * nobjects);

    if (markerType == 0){
        objects[nobjects - 1].id = pattid;
    } else {
        // Dado que no se obtiene el número identidad del multipatrón, se asigna manualmente
        objects[nobjects - 1].id = 999;
    }
    objects[nobjects - 1].width = w;
    objects[nobjects - 1].center[0] = c[0];
    objects[nobjects - 1].center[1] = c[1];
    objects[nobjects - 1].markerType = markerType;  // markerType == 0 si es un patron simple, markerType== 0 si es multipatron
    objects[nobjects - 1].drawme = drawme;
}

// ==== draw****** (Dibujado especifico de cada objeto) =============
void drawOxigen(int nobject)
{
    double gl_para[16]; // Esta matriz 4x4 es la usada por OpenGL
    argConvGlpara(objects[nobject].patt_trans, gl_para);
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixd(gl_para);


    // Renderiza un átomo de oxígeno
    glTranslatef(0.0, 0.0, heightZ);
    glColor3ub(255, 0, 0);
    glutWireSphere(radAtom, defSlices, defStacks);

    // Renderiza sus electrones
    for (int i = 0; i < 6; i++)
    {
        glPushMatrix();
        glRotatef(electrones[i], 0.0, 0.0, 1.0); // Rotación del electrón alrededor del átomo
        glRotatef(i * 45.0, 0.0, 0.0, 1.0);      // Rotación para posicionar el electrón alrededor del átomo
        glTranslatef(radAtom * 2.0, 0.0, 0.0);        // Distancia del electrón al átomo

        // Renderiza el electrón
        glColor3ub(0, 0, 0);
        glutWireSphere(radElectron, defSlices, defStacks);

        glPopMatrix();
    }
}

void drawHydrogen(int nobject)
{
    double gl_para[16]; // Esta matriz 4x4 es la usada por OpenGL
    argConvGlpara(objects[nobject].patt_trans, gl_para);
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixd(gl_para);

    // Renderiza un átomo de hidrógeno
    glTranslatef(0.0, 0.0, heightZ);
    glColor3ub(255, 255, 255);
    glutWireSphere(radAtom, defSlices, defStacks);

    // Renderiza sus electrones
    for (int i = 0; i < 2; i++)
    {
        glPushMatrix();
        glRotatef(electrones[i], 0.0, 0.0, 1.0); // Rotación del electrón alrededor del átomo
        glRotatef(i * 45.0, 0.0, 0.0, 1.0);      // Rotación para posicionar el electrón alrededor del átomo
        glTranslatef(radAtom * 2.0, 0.0, 0.0);        // Distancia del electrón al átomo

        // Renderiza el electrón
        glColor3ub(0, 0, 0);
        glutWireSphere(radElectron, defSlices, defStacks);

        glPopMatrix();
    }
}

void drawDioxygen(double x_axis, double y_axis)
{
    glTranslatef(x_axis, y_axis, 0.0);

    // Renderiza un átomo de oxígeno
    glPushMatrix();
    glTranslatef(0.0, distComAtom2, heightZ);
    glColor3ub(255, 0, 0);
    glutWireSphere(radComAtom, defSlices, defStacks);
    glPopMatrix();

    // Renderiza un átomo de oxígeno
    glPushMatrix();
    glTranslatef(0.0, -distComAtom2, heightZ);
    glColor3ub(255, 0, 0);
    glutWireSphere(radComAtom, defSlices, defStacks);
    glPopMatrix();
}

void drawOxidane(double x_axis, double y_axis)
{
    glTranslatef(x_axis, y_axis, 0.0);

    // Renderiza un átomo de oxígeno
    glPushMatrix();
    glTranslatef(0.0, 0.0, heightZ);
    glColor3ub(255, 0, 0);
    glutWireSphere(radComAtom, defSlices, defStacks);
    glPopMatrix();

    // Renderiza un átomo de hidrógeno
    glPushMatrix();
    glTranslatef(0, -distComAtom1, lessHeightZ);
    glColor3ub(255, 255, 255);
    glutWireSphere(radAtom, defSlices, defStacks);
    glPopMatrix();

    // Renderiza un átomo de hidrógeno
    glPushMatrix();
    glTranslatef(0, distComAtom1, lessHeightZ);
    glColor3ub(255, 255, 255);
    glutWireSphere(radAtom, defSlices, defStacks);
    glPopMatrix();
}

void drawCarbonDioxide(double x_axis, double y_axis)
{
    glTranslatef(x_axis, y_axis, 0.0);

    // Renderiza un átomo de oxígeno
    glPushMatrix();
    glTranslatef(0.0, distComAtom2, heightZ);
    glColor3ub(255, 0, 0);
    glutWireSphere(radComAtom, defSlices, defStacks);
    glPopMatrix();

    // Renderiza un átomo negro
    glPushMatrix();
    glTranslatef(0.0, 0.0, heightZ);
    glColor3ub(0, 0, 0);
    glutWireSphere(18, defSlices, defStacks);
    glPopMatrix();

    // Renderiza un átomo de oxígeno
    glPushMatrix();
    glTranslatef(0.0, -distComAtom2, heightZ);
    glColor3ub(255, 0, 0);
    glutWireSphere(radComAtom, defSlices, defStacks);
    glPopMatrix();
}

void drawOzone(double x_axis, double y_axis)
{
    glTranslatef(x_axis, y_axis, 0.0);

    // Renderiza un átomo de oxígeno
    glPushMatrix();
    glTranslatef(0.0, 0.0, heightZ);
    glColor3ub(240, 0, 0);
    glutWireSphere(radComAtom, defSlices, defStacks);
    glPopMatrix();

    // Renderiza un átomo de oxígeno
    glPushMatrix();
    glTranslatef(0, -distComAtom3, lessHeightZ);
    glColor3ub(255, 0, 0);
    glutWireSphere(radComAtom, defSlices, defStacks);
    glPopMatrix();

    // Renderiza un átomo de oxígeno
    glPushMatrix();
    glTranslatef(0, distComAtom3, lessHeightZ);
    glColor3ub(255, 0, 0);
    glutWireSphere(radComAtom, defSlices, defStacks);
    glPopMatrix();
}

// Metodo para dibujar atomos compuestos en el multipatron
void drawMulti(int nobject)
{
    double gl_para[16]; // Esta matriz 4x4 es la usada por OpenGL

    argConvGlpara(mMarker->trans, gl_para);
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixd(gl_para);

    // Se recorren todos los patrones del multipatron
    for (int i = 0; i < mMarker->marker_num && i < nobjectsMulti; i++)
    {
        glPushMatrix(); // Guardamos la matriz actual
        argConvGlpara(mMarker->marker[i].trans, gl_para);
        glMultMatrixd(gl_para);
        objectsMulti[i].drawmeM(i, i); // Se dibuja el atomo compuesto
        glPopMatrix(); // Recuperamos la matriz anterior
    }
}

// Metodo para dibujar atomos compuestos en funcion de la rotacion del patron ROTPAT
void drawAtoms(double m[3][4], double distance)
{
    if (rotZ == DOWN){ // La marca ROTPAT está del revés (180 grados)
        drawDioxygen((fabs(m[0][3]) > thresholdM) ? trunc(m[0][3]/2) : 0, (fabs(m[1][3]) > thresholdM) ? trunc(m[1][3]/2) : 0);
    } else if (rotZ == LEFT){ // La marca ROTPAT está girada hacia la izquierda
        drawOzone((fabs(m[0][3]) > thresholdM) ? trunc(m[0][3]/2) : 0, (fabs(m[1][3]) > thresholdM) ? trunc(m[1][3]/2) : 0);
    } else if (rotZ == RIGHT){ // La marca ROTPAT está girada hacia la derecha
        drawCarbonDioxide((fabs(m[0][3]) > thresholdM) ? trunc(m[0][3]/2) : 0, (fabs(m[1][3]) > thresholdM) ? trunc(m[1][3]/2) : 0);
    } else { // La marca ROTPAT está girada hacia cualquier otra direccion
        drawOxidane((fabs(m[0][3]) > thresholdM) ? trunc(m[0][3]/2) : 0, (fabs(m[1][3]) > thresholdM) ? trunc(m[1][3]/2) : 0);
    }
}

// Metodo para añadir atomos compuestos a la lista del multipatron
void addAtomsMulti()
{
    if (rotZ == DOWN){
        addObjectMulti(drawDioxygen);
        printf("Molécula de O2 añadida a la lista de objetos del multipatrón.\n");
    } else if (rotZ == LEFT){ 
        addObjectMulti(drawOzone);
        printf("Molécula de O2 añadida a la lista de objetos del multipatrón.\n");    
    } else if (rotZ == RIGHT){
        addObjectMulti(drawCarbonDioxide);
        printf("Molécula de CO2 añadida a la lista de objetos del multipatrón.\n");
    } else {
        addObjectMulti(drawOxidane);
        printf("Molécula de agua añadida a la lista de objetos del multipatrón.\n");
    }
}


// ======== draw ====================================================
void draw(void)
{
    double gl_para[16]; // Esta matriz 4x4 es la usada por OpenGL

    argDrawMode3D();              // Cambiamos el contexto a 3D
    argDraw3dCamera(0, 0);        // Y la vista de la camara a 3D
    glClear(GL_DEPTH_BUFFER_BIT); // Limpiamos buffer de profundidad
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    double dist01; // Distancia entre el objeto 0 y 1
    double m[3][4], m2[3][4];   // Auxiliares para el calculo de la distancia

    if (objects[ATOMPAT1].visible && objects[ATOMPAT2].visible) // Si ambos patrones son visibles, se comprueba la distancia entre ellos
    {
        argConvGlpara(objects[ATOMPAT1].patt_trans, gl_para);
        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixd(gl_para);

        arUtilMatInv(objects[ATOMPAT1].patt_trans, m);
        arUtilMatMul(m, objects[ATOMPAT2].patt_trans, m2);

        dist01 = sqrt(pow(m2[0][3], 2) + pow(m2[1][3], 2) + pow(m2[2][3], 2)); // Obtenemos la distancia entre ambos patrones

        // Si la distancia es menor a la distancia constante de referencia, se dibuja el atomo combinado
        if (dist01 < dist01Const)
        {
            if (objects[ROTPAT].visible){ // Marca de rotación
                obtainZangle(ROTPAT);   // Obtenemos el angulo de giro en el eje z de la marca
                objects[ROTPAT].timer=time(NULL);
                fixed = 0;
                
            } else if (objects[ROTPAT].timer > 0 && (time(NULL) - objects[ROTPAT].timer) > selectionTime && fixed == 0){ 
                // Si ha sido visto el patron de rotacion y la ultima vez ha sido hace mas tiempo que el selectionTime, se confirma el patron para el patron multimarca
                fixed = 1;
                printf("Patrón seleccionado.\n");
                addAtomsMulti();
            }

            drawAtoms(m2, dist01);
        }
        else    // Si no, se dibujan por separado
        {
            objects[ATOMPAT1].drawme(ATOMPAT1); // Llamamos a su función de dibujar
            objects[ATOMPAT2].drawme(ATOMPAT2); // Llamamos a su función de dibujar
        }
    }
    else if (objects[ATOMPAT1].visible)
    {
        objects[ATOMPAT1].drawme(ATOMPAT1); // Llamamos a su función de dibujar
    }
    else if (objects[ATOMPAT2].visible)
    {
        objects[ATOMPAT2].drawme(ATOMPAT2); // Llamamos a su función de dibujar
    } 
    
    if (objects[MULTPAT].visible)  // Multipatrón
    {
        objects[MULTPAT].drawme(3); // Llamamos a su función de dibujar
    }

    update();

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

    // Inicializamos la lista de patrones
    addObject("data/simple.patt", 85.0, c, 0, drawOxigen);      //  ATOMPAT1
    addObject("data/4x4_28.patt", 85.0, c, 0, drawHydrogen);    //  ATOMPAT2
    addObject("data/4x4_79.patt", 85.0, c, 0, NULL);            //  ROTPAT
    addObject("data/marker.dat", 0, c, 1, drawMulti);           //  MULTPAT

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
        { // Si ha detectado el patron (no multimarca)
            objects[i].visible = 1;
            if (objects[i].markerType == 0){
                arGetTransMat(&marker_info[k], objects[i].center, objects[i].width, objects[i].patt_trans);
            }
        }
        else
        {   // El objeto no es visible o es multimarca
            objects[i].visible = 0;

            // Reconocimiento patron multimarca
            if(arMultiGetTransMat(marker_info, marker_num, mMarker) > 0) {
                objects[i].visible = 1;
            }
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
    return (0);
}
