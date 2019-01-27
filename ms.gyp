{
    'variables': {
        'src': [
            'src/mongoose.c',
            'src/ms_server.c',
            'src/ms_session.c',
            'src/ms_task.c',
            'src/ms_http_pipe.c',
            'src/ms_mem_storage.c',
            'src/ms_file_storage.c',
            'src/ms_memory_pool.c',
        ],
        'test_src': [
            'test/fake_file.c',
            'test/fake_pipe.c',
            'test/fake_reader.c',
            'test/fake_server.c',
            'test/test_file_storage.c',
            'test/test_mem_storage.c',
            'test/test_server.c',
            'test/test_task.c',
            'test/test_main.c',
            'test/run_tests.c',
        ]
    },
    'targets': [
        {
            'target_name': 'ms_test',
            'type': 'executable',
            'msvs_guid': 'A2BDD354-2C49-49F8-9613-1A31BBB39E5F',
            'include_dirs': [
                'include',
                'src/',
                'test/'
            ],
            'sources': [
                '<@(src)',
                '<@(test_src)',
            ],
            'link_settings': {
                'libraries': ['-lpthread', '-lm', '--coverage']
            },
            'cflags': [
                '--coverage'
            ]
        },
    ],
}
