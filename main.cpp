// DisasterScape - Natural Disaster Simulator (Flood + Earthquake)
#include <GL/freeglut.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <algorithm>
using namespace std;

const float PI = 3.14159265f;
enum Disaster { NORMAL, FLOOD, EARTHQUAKE };
enum PType { RAIN, DEBRIS };
struct Particle { float x, y, z, vx, vy, vz, life, size; PType type; };
struct Building { float x, z, w, h, d, r, g, b; };
struct Crack { vector<float> x, z; };

Disaster current = NORMAL;
bool paused = false;
float simTime = 0, cloudOff = 0;
float camAngle = 25, camHeight = 14, camDist = 42;
int winW = 1000, winH = 650;
vector<Particle> parts;
vector<Crack> cracks;

const int NB = 5;
Building blds[NB] = {
    {-12, -10, 5, 8, 5, 0.80f, 0.55f, 0.45f},
    {-4, -12, 4, 13, 4, 0.60f, 0.65f, 0.80f},
    {6, -10, 6, 6, 5, 0.90f, 0.80f, 0.50f},
    {-14, 9, 5, 5, 5, 0.75f, 0.45f, 0.45f},
    {9, 10, 5, 9, 5, 0.50f, 0.70f, 0.60f}};
const int NT = 7;
float treePos[NT][2] = {{-22,-4},{-8,5},{3,7},{16,-5},{22,6},{-2,-18},{12,-16}};
const char* names[] = {"Normal", "Flood", "Earthquake"};

// ---------- helpers ----------
float frand(float a, float b) { return a + (b - a) * (rand() / (float)RAND_MAX); }
float lerpf(float a, float b, float t) { return a + (b - a) * t; }
float clamp01(float v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }
float prog() { return clamp01(simTime / 10.0f); }

void addP(PType t, float x, float y, float z, float vx, float vy, float vz, float life, float size) {
    Particle p = {x, y, z, vx, vy, vz, life, size, t};
    parts.push_back(p);
}

void resetSim() {
    simTime = 0;
    parts.clear();
}

void initCracks() {
    for (int i = 0; i < 22; i++) {
        Crack c;
        float px = frand(-35, 35), pz = frand(-35, 35);
        for (int j = 0; j < 7; j++) {
            c.x.push_back(px); c.z.push_back(pz);
            px += frand(-2.5f, 2.5f); pz += frand(-2.5f, 2.5f);
        }
        cracks.push_back(c);
    }
}

// ---------- scene objects ----------
void drawGround() {
    float k = prog();
    float r = 0.25f, g = 0.62f, b = 0.22f;
    if (current == FLOOD) { r *= 1 - 0.2f * k; g *= 1 - 0.2f * k; b *= 1 - 0.2f * k; }
    glNormal3f(0, 1, 0);
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
    glVertex3f(-40, 0, -40); glVertex3f(-40, 0, 40); glVertex3f(40, 0, 40); glVertex3f(40, 0, -40);
    glEnd();
    // road
    glColor3f(0.2f, 0.2f, 0.2f);
    glBegin(GL_QUADS);
    glVertex3f(-40, 0.02f, -3); glVertex3f(-40, 0.02f, 3); glVertex3f(40, 0.02f, 3); glVertex3f(40, 0.02f, -3);
    glEnd();
    glColor3f(0.9f, 0.9f, 0.9f);
    glBegin(GL_QUADS);
    for (float x = -38; x < 38; x += 6) {
        glVertex3f(x, 0.04f, -0.15f); glVertex3f(x, 0.04f, 0.15f);
        glVertex3f(x + 3, 0.04f, 0.15f); glVertex3f(x + 3, 0.04f, -0.15f);
    }
    glEnd();
}

void drawPond() {
    float R = 6.0f;
    glNormal3f(0, 1, 0);
    glColor3f(0.2f, 0.5f, 0.85f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(20, 0.03f, 14);
    for (int i = 0; i <= 32; i++) {
        float a = i * 2 * PI / 32;
        glVertex3f(20 + R * cosf(a), 0.03f, 14 + R * sinf(a));
    }
    glEnd();
}

void drawCracks() {
    float p = 0;
    if (current == EARTHQUAKE && simTime > 2) p = clamp01((simTime - 2) / 3);
    if (p <= 0) return;
    glDisable(GL_LIGHTING);
    glColor3f(0.2f, 0.13f, 0.07f);
    glLineWidth(2);
    for (size_t i = 0; i < cracks.size(); i++) {
        int n = max(2, (int)(cracks[i].x.size() * p + 0.5f));
        glBegin(GL_LINE_STRIP);
        for (int j = 0; j < n; j++) glVertex3f(cracks[i].x[j], 0.06f, cracks[i].z[j]);
        glEnd();
    }
    glEnable(GL_LIGHTING);
}

void drawTree(float x, float z, int idx) {
    glPushMatrix();
    glTranslatef(x, 0, z);
    glColor3f(0.4f, 0.26f, 0.10f);
    glPushMatrix(); glTranslatef(0, 1.2f, 0); glScalef(0.5f, 2.4f, 0.5f); glutSolidCube(1); glPopMatrix();
    glColor3f(0.10f, 0.50f, 0.15f);
    glPushMatrix(); glTranslatef(0, 2.2f, 0); glRotatef(-90, 1, 0, 0); glutSolidCone(2.0, 4.0, 12, 2); glPopMatrix();
    glPushMatrix(); glTranslatef(0, 4.2f, 0); glRotatef(-90, 1, 0, 0); glutSolidCone(1.5, 3.0, 12, 2); glPopMatrix();
    glPopMatrix();
}

void drawBuilding(const Building& b, int idx) {
    float shake = 0, tilt = 0;
    if (current == EARTHQUAKE) {
        float a = clamp01(simTime / 6);
        shake = sinf(simTime * 40 + idx) * 0.15f * a;       // TRANSLATION
        tilt = sinf(simTime * 25 + idx * 2) * 2.0f * a;     // ROTATION
    }
    glPushMatrix();
    glTranslatef(b.x + shake, 0, b.z);
    glRotatef(tilt, 0, 0, 1);
    glColor3f(b.r, b.g, b.b);
    glPushMatrix(); glTranslatef(0, b.h / 2, 0); glScalef(b.w, b.h, b.d); glutSolidCube(1); glPopMatrix();   // SCALING
    glColor3f(b.r * 0.6f, b.g * 0.6f, b.b * 0.6f);
    glPushMatrix(); glTranslatef(0, b.h + 0.2f, 0); glScalef(b.w + 0.4f, 0.4f, b.d + 0.4f); glutSolidCube(1); glPopMatrix();
    glDisable(GL_LIGHTING);
    bool lit = (current == FLOOD);
    glColor3f(lit ? 1.0f : 0.7f, lit ? 0.9f : 0.88f, lit ? 0.55f : 0.97f);
    glBegin(GL_QUADS);
    for (float wy = 1.5f; wy < b.h - 1; wy += 2.2f)
        for (float wx = -b.w / 2 + 0.8f; wx < b.w / 2 - 1.2f; wx += 1.6f) {
            float zz = b.d / 2 + 0.02f;
            glVertex3f(wx, wy, zz); glVertex3f(wx + 0.8f, wy, zz);
            glVertex3f(wx + 0.8f, wy + 1, zz); glVertex3f(wx, wy + 1, zz);
        }
    glEnd();
    if (current == EARTHQUAKE && simTime > 2) {      // building crack
        float a = clamp01((simTime - 2) / 3);
        float zz = b.d / 2 + 0.04f;
        glColor3f(0.05f, 0.05f, 0.05f); glLineWidth(2.5f);
        glBegin(GL_LINE_STRIP);
        glVertex3f(-b.w * 0.2f, b.h, zz);
        glVertex3f(-b.w * 0.2f + 0.6f * a, b.h - b.h * 0.25f * a, zz);
        glVertex3f(-b.w * 0.2f - 0.4f * a, b.h - b.h * 0.5f * a, zz);
        glVertex3f(-b.w * 0.2f + 0.8f * a, b.h - b.h * 0.75f * a, zz);
        glEnd();
    }
    glEnable(GL_LIGHTING);
    glPopMatrix();
}

void drawClouds() {
    float k = prog();
    float c = (current == FLOOD) ? lerpf(1.0f, 0.5f, k) : 1.0f;
    float sc = (current == FLOOD) ? 1 + 0.5f * k : 1;
    float bx[5] = {-40, -15, 10, 35, 55}, bz[5] = {-25, -10, -30, 5, -18}, by[5] = {26, 29, 25, 28, 27};
    glColor3f(c, c, c);
    for (int i = 0; i < 5; i++) {
        float x = fmodf(bx[i] + cloudOff + 80, 160) - 80;   // TRANSLATION: cloud soro
        glPushMatrix();
        glTranslatef(x, by[i], bz[i]);
        glScalef(sc, sc, sc);
        glutSolidSphere(4, 12, 12);
        glTranslatef(4, 0.8f, 0); glutSolidSphere(5, 12, 12);
        glTranslatef(5, -0.8f, 0); glutSolidSphere(4, 12, 12);
        glPopMatrix();
    }
}

void drawSun() {
    if (current == FLOOD) return;
    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 0.9f, 0.3f);
    glPushMatrix();
    glTranslatef(30, 30, -45);
    glutSolidSphere(3, 16, 16);
    glPopMatrix();
    glEnable(GL_LIGHTING);
}

void drawFloodWater() {
    float wy = min(simTime * 0.4f, 4.5f);              // pani er level barche
    // vasha jinis (opaque, tai age)
    glColor3f(0.55f, 0.35f, 0.17f);
    glPushMatrix(); glTranslatef(4 + sinf(simTime * 0.5f) * 3, wy + 0.3f + 0.2f * sinf(simTime * 2.5f), 2); glRotatef(simTime * 20, 0, 1, 0); glutSolidCube(1.6); glPopMatrix();
    glColor3f(0.75f, 0.25f, 0.25f);
    glPushMatrix(); glTranslatef(-16 + sinf(simTime * 0.4f) * 2, wy + 0.3f + 0.2f * sinf(simTime * 2.0f + 1), -3); glRotatef(-simTime * 15, 0, 1, 0); glutSolidCube(1.4); glPopMatrix();
    glDisable(GL_LIGHTING);
    glDepthMask(GL_FALSE);
    glColor4f(0.15f, 0.4f, 0.8f, 0.6f);
    glBegin(GL_QUADS);
    for (float x = -40; x < 40; x += 4)
        for (float z = -40; z < 40; z += 4) {
            float y00 = wy + 0.12f * sinf(x * 0.5f + z * 0.4f + simTime * 3);
            float y10 = wy + 0.12f * sinf((x + 4) * 0.5f + z * 0.4f + simTime * 3);
            float y11 = wy + 0.12f * sinf((x + 4) * 0.5f + (z + 4) * 0.4f + simTime * 3);
            float y01 = wy + 0.12f * sinf(x * 0.5f + (z + 4) * 0.4f + simTime * 3);
            glVertex3f(x, y00, z); glVertex3f(x + 4, y10, z); glVertex3f(x + 4, y11, z + 4); glVertex3f(x, y01, z + 4);
        }
    glEnd();
    glDepthMask(GL_TRUE);
    glEnable(GL_LIGHTING);
}

void drawParticles() {
    glDisable(GL_LIGHTING);
    glEnable(GL_POINT_SMOOTH);
    glLineWidth(1);
    glColor4f(0.7f, 0.8f, 1.0f, 0.7f);
    glBegin(GL_LINES);
    for (size_t i = 0; i < parts.size(); i++) {
        const Particle& p = parts[i];
        if (p.type != RAIN) continue;
        glVertex3f(p.x, p.y, p.z);
        glVertex3f(p.x - p.vx * 0.04f, p.y - p.vy * 0.04f, p.z - p.vz * 0.04f);
    }
    glEnd();
    for (size_t i = 0; i < parts.size(); i++) {
        const Particle& p = parts[i];
        if (p.type != DEBRIS) continue;
        glColor4f(0.4f, 0.38f, 0.34f, 1);
        glPointSize(5);
        glBegin(GL_POINTS); glVertex3f(p.x, p.y, p.z); glEnd();
    }
    glEnable(GL_LIGHTING);
}

void drawText(float x, float y, const char* s) {
    glRasterPos2f(x, y);
    for (; *s; s++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *s);
}

void drawOverlay() {
    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); gluOrtho2D(0, winW, 0, winH);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    glColor4f(0, 0, 0, 0.45f);
    glBegin(GL_QUADS); glVertex2f(0, winH - 44); glVertex2f(winW, winH - 44); glVertex2f(winW, winH); glVertex2f(0, winH); glEnd();
    char buf[128];
    sprintf(buf, "DisasterScape | %s | t = %.1fs %s", names[current], simTime, paused ? "(paused)" : "");
    glColor3f(1, 1, 1);
    drawText(10, winH - 18, buf);
    drawText(10, winH - 36, "1 Flood  2 Earthquake  0/R Reset  Space Pause  Arrows Rotate  +/- Zoom");
    glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST); glEnable(GL_LIGHTING);
}

// ---------- simulation update ----------
void updateSim(float dt) {
    if (current == FLOOD) {
        for (int i = 0; i < 12; i++)
            addP(RAIN, frand(-40, 40), 30, frand(-40, 40), -2.0f, -frand(25, 32), 0, 1, 0);
    }
    if (current == EARTHQUAKE && simTime > 2) {
        for (int i = 0; i < NB; i++)
            if (rand() % 5 == 0)
                addP(DEBRIS, blds[i].x + frand(-blds[i].w / 2, blds[i].w / 2), blds[i].h * frand(0.6f, 1.0f), blds[i].z + blds[i].d / 2,
                     frand(-1, 1), 0, frand(0, 1.5f), 1, 5);
    }
    for (size_t i = 0; i < parts.size(); i++) {
        Particle& p = parts[i];
        if (p.type == DEBRIS) p.vy -= 15 * dt;
        p.x += p.vx * dt; p.y += p.vy * dt; p.z += p.vz * dt;
        float decay = (p.type == DEBRIS) ? 0.25f : 0;
        p.life -= decay * dt;
    }
    parts.erase(remove_if(parts.begin(), parts.end(),
        [](const Particle& p) { return p.life <= 0 || p.y < 0 || p.y > 40 || fabsf(p.x) > 60 || fabsf(p.z) > 60; }), parts.end());
}

// ---------- GLUT callbacks ----------
void display() {
    float k = (current == NORMAL) ? 0 : prog();
    float sr = 0.53f, sg = 0.81f, sb = 0.92f, tr = sr, tg = sg, tb = sb;
    if (current == FLOOD) { tr = 0.41f; tg = 0.45f; tb = 0.52f; }
    glClearColor(lerpf(sr, tr, k), lerpf(sg, tg, k), lerpf(sb, tb, k), 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    float sx = 0, sy = 0;
    if (current == EARTHQUAKE) { float a = clamp01(simTime / 6) * 0.4f; sx = frand(-1, 1) * a; sy = frand(-1, 1) * a; }
    float rad = camAngle * PI / 180;
    gluLookAt(camDist * sinf(rad) + sx, camHeight + sy, camDist * cosf(rad), 0, 3, 0, 0, 1, 0);

    float li = (current == FLOOD) ? lerpf(1.0f, 0.55f, k) : 1.0f;
    GLfloat lp[] = {20, 40, 20, 0}, diff[] = {li, li, li, 1}, amb[] = {0.35f, 0.35f, 0.35f, 1};
    glLightfv(GL_LIGHT0, GL_POSITION, lp);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diff);
    glLightfv(GL_LIGHT0, GL_AMBIENT, amb);

    drawSun();
    drawClouds();
    drawGround();
    drawPond();
    drawCracks();
    for (int i = 0; i < NT; i++) drawTree(treePos[i][0], treePos[i][1], i);
    for (int i = 0; i < NB; i++) drawBuilding(blds[i], i);
    if (current == FLOOD) drawFloodWater();
    drawParticles();
    drawOverlay();
    glutSwapBuffers();
}

void timer(int) {
    const float dt = 0.016f;
    if (!paused) {
        float sp = (current == FLOOD) ? 2.5f : 0.6f;
        cloudOff += sp * dt;
        if (current != NORMAL) { simTime += dt; updateSim(dt); }
    }
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

void setMode(Disaster d) { current = d; resetSim(); paused = false; }

void keyboard(unsigned char key, int, int) {
    switch (key) {
        case '1': setMode(FLOOD); break;
        case '2': setMode(EARTHQUAKE); break;
        case '0': case 'r': case 'R': setMode(NORMAL); break;
        case ' ': paused = !paused; break;
        case '+': case '=': camDist = max(15.0f, camDist - 2); break;
        case '-': camDist = min(80.0f, camDist + 2); break;
        case 27: exit(0);
    }
}

void special(int key, int, int) {
    if (key == GLUT_KEY_LEFT) camAngle -= 3;
    if (key == GLUT_KEY_RIGHT) camAngle += 3;
    if (key == GLUT_KEY_UP) camHeight = min(40.0f, camHeight + 1);
    if (key == GLUT_KEY_DOWN) camHeight = max(3.0f, camHeight - 1);
}

void wheel(int, int dir, int, int) { camDist = max(15.0f, min(80.0f, camDist - dir * 2)); }

void reshape(int w, int h) {
    if (h == 0) h = 1;
    winW = w; winH = h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluPerspective(55, (double)w / h, 0.5, 300);
    glMatrixMode(GL_MODELVIEW);
}

void initGL() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING); glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_NORMALIZE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glShadeModel(GL_SMOOTH);
}

int main(int argc, char** argv) {
    srand((unsigned)time(0));
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(winW, winH);
    glutCreateWindow("DisasterScape - Natural Disaster Simulator");
    initGL();
    initCracks();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special);
    glutMouseWheelFunc(wheel);
    glutTimerFunc(16, timer, 0);
    glutMainLoop();
    return 0;
}
