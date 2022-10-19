{
    "targets": [
        {
            "target_name": "ioctl",
            "sources": [
                "../src/ioctl.addon.cc",
            ],
            "include_dirs" : [
                "<!(node -e \"require('nan')\")",
            ],
            "conditions": [
                [
                    'OS=="mac"',
                    {
                        "libraries": ["-L/opt/homebrew/lib"],
                        "include_dirs": ["/opt/homebrew/include"],
                    },
                ],
            ],
        }
    ]
}
