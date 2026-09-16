#ifdef __cplusplus
extern "C" {
#endif

void Spectrogrammer_Init(void *window);
void Spectrogrammer_Shutdown();
void Spectrogrammer_InvalidateGraphics();
bool Spectrogrammer_MainLoopStep();
bool Spectrogrammer_ShouldAnimate();

#ifdef __cplusplus
}
#endif