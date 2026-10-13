/*
 * Compatibilite : raylib 5.5 appelle cette fonction GLFW, absente des anciennes
 * versions d'Emscripten. Elle ne sert qu'a l'ecran haute densite : on la neutralise.
 */
void *glfwSetWindowContentScaleCallback(void *fenetre, void *rappel)
{
    (void)fenetre;
    (void)rappel;
    return 0;
}
