all:
	cc -o tpod -l mpg123 -l ao -l curl -l m tpod.c
clean:
	rm tpod
