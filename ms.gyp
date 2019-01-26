{
    'variables': {
        'src': [
            'mongoose.c',
            'ms_server.c',
            'ms_session.c',
            'ms_task.c',
            'ms_http_pipe.c',
            'ms_mem_storage.c',
            'ms_file_storage.c',
            'ms_memory_pool.c',
        ],
        'test_src': [
            'fake_file.c',
            'fake_pipe.c',
            'fake_reader.c',
            'fake_server.c',
            'test_file_storage.c',
            'test_mem_storage.c',
            'test_server.c',
            'test_task.c',
            'test_main.c'
            'run_tests.c',
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
        },
    ],
}
