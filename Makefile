all:
	cc -o tpod -l sqlite3 -l mpg123 -l ao -l curl -l mrss -l pthread src/mongoose/mongoose.c -D MG_ENABLE_THREADS src/tpod.c

clean:
	rm tpod
