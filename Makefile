%: %.c
	# gcc -o $@ -O3 -lm -lGL -lGLU -lglut -Wall $<
	gcc -o cgv cgv.c -framework Carbon -framework OpenGL -framework GLUT -mmacosx-version-min=10.14
