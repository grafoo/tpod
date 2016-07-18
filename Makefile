all:
	cc -o tpod -l mpg123 -l ao -l curl -l m -l pthread mongoose/mongoose.c -D MG_ENABLE_THREADS tpod.c

clean:
	rm tpod
