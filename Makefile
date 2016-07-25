all:
	cc -o tpod src/tpod.c src/mongoose/mongoose.c -D MG_ENABLE_THREADS -l pthread -l jansson -l sqlite3 -l mpg123 -l ao -l curl -l mrss

clean:
	rm tpod
