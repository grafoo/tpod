Program(
    'tpod',
    ['src/tpod.c', 'src/mongoose/mongoose.c'],
    LIBS=[
        'pthread',
        'jansson',
        'sqlite3',
        'mpg123',
        'ao',
        'curl',
        'mrss'
    ],
    LIBPATH=['dep/lib64'],
    CPPPATH=['dep/include'],
    CCFLAGS=['-D MG_ENABLE_THREADS']
)
