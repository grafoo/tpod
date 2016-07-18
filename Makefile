all:
	cc -o tpod -l mpg123 -l ao -l curl -l mrss -l pthread mongoose/mongoose.c -D MG_ENABLE_THREADS src/tpod.c

clean:
	rm tpod
