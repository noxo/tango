#include <postproc/medianer.h>
#include <GL/freeglut.h>
#include <unistd.h>

#define DATASET_OBJ "dataset/1529165036844.obj"
#define DATASET_PATH "dataset"

oc::Medianer* medianer;
int pose = 0;

void display(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    medianer->RenderPose(pose++);
    if (pose > medianer->GetPoseCount())
        pose = 0;
    sleep(1);
    glutSwapBuffers();
    glutPostRedisplay();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitWindowSize(960, 540);
    glutInitContextVersion(3,0);
    glutInitContextProfile(GLUT_CORE_PROFILE);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutCreateWindow("Test");
    glutDisplayFunc(display);

    medianer = new oc::Medianer(DATASET_PATH, DATASET_OBJ);
    for (int pass = oc::PASS_SUMMARY; pass < oc::PASS_SAVE; pass++)
        for (int i = 0; i <= medianer->GetPoseCount(); i++)
            medianer->RenderTexture(i, pass);
    glutMainLoop();
    return 0;
}
