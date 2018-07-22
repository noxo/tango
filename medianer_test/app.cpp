#include <postproc/medianer.h>
#include <GL/freeglut.h>

#define DATASET_OBJ "1529165036844.obj"
#define DATASET_PATH "dataset"

oc::Medianer* medianer;
int pose = 0;

void display(void) {
    medianer->RenderTexture(pose++);
    medianer->RenderPose(5);

    if (pose >= medianer->GetPoseCount())
        pose = 0;
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

    medianer = new oc::Medianer(DATASET_PATH, DATASET_OBJ, true);
    glutMainLoop();
    return 0;
}
