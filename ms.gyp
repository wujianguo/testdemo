{
    'variables': {
        'src': [
            'include/ms.h',
            'include/queue.h',
            'include/mongoose.h',
            'src/mongoose.c',
            'src/ms_server.h',
            'src/ms_server.c',
            'src/ms_session.h',
            'src/ms_session.c',
            'src/ms_task.h',
            'src/ms_task.c',
            'src/ms_http_pipe.h',
            'src/ms_http_pipe.c',
            'src/ms_mem_storage.h',
            'src/ms_mem_storage.c',
            'src/ms_file_storage.h',
            'src/ms_file_storage.c',
            'src/ms_memory_pool.h',
            'src/ms_memory_pool.c',
        ],
        'test_src': [
            'test/fake_file.h',
            'test/fake_file.c',
            'test/fake_pipe.h',
            'test/fake_pipe.c',
            'test/fake_reader.h',
            'test/fake_reader.c',
            'test/fake_server.h',
            'test/fake_server.c',
            'test/test_file_storage.c',
            'test/test_mem_storage.c',
            'test/test_server.c',
            'test/test_task.c',
            'test/test_main.c',
            'test/run_tests.h',
            'test/run_tests.c',
        ]
    },
    'target_defaults': {
        'default_configuration': 'Debug',
        'configurations': {
            'Debug': {
                'defines': ['DEBUG', '_DEBUG'],
                'cflags': ['-g'],
                'xcode_settings': {
                    'GCC_OPTIMIZATION_LEVEL': '0',
                },
            },
            'Release': {
                'defines': ['NDEBUG'],
                'cflags': [
                    '-O3',
                ],
            }
        }
    },
    'targets': [
        {
            'target_name': 'libms',
            'type': 'static_library',
            'include_dirs': [
                'include',
                'src/',
            ],
            'sources': [
                '<@(src)',
            ],
            'conditions': [
                ['OS == "linux"', {
                    'link_settings': {
                        'libraries': ['-lpthread', '-lm']
                    },
                }],
            ]
        },
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
            'conditions': [
                ['OS == "linux"', {
                    'link_settings': {
                        'libraries': ['-lpthread', '-lm', '--coverage']
                    },
                    'cflags': [
                        '--coverage'
                    ]
                }],
            ],
        },
    ],
}
