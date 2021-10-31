vars = {
  # Internal repositories.
  'microsoft_chromium_git': 'https://domoreexp.visualstudio.com/DefaultCollection/Teamspace/_git/electron-chromium',
  'microsoft_depot_tools_git': 'https://domoreexp.visualstudio.com/DefaultCollection/Teamspace/_git/electron-depot-tools',
  'microsoft_electron_git': 'https://domoreexp.visualstudio.com/DefaultCollection/Teamspace/_git/electron-electron',
  'microsoft_node_git': 'https://domoreexp.visualstudio.com/DefaultCollection/Teamspace/_git/electron-node',

  # Electron checkout configuration.
  # It should match the upstream and contents of the "gclient_gn_args" below.
  'build_with_chromium': True,
  'checkout_android': False,
  'checkout_android_native_support': False,
  'checkout_google_benchmark': False,
  'checkout_libaom': True,
  'checkout_nacl': False,
  'checkout_oculus_sdk': False,
  'checkout_openxr': False,
  'checkout_pgo_profiles': True,
  'mac_xcode_version': 'default',

  # Internal checkout configuration.
  'microsoft_add_custom_ffmpeg_configs': True,
  'microsoft_apply_customization_patches': True,
  'microsoft_download_external_binaries': True,
  'microsoft_use_internal_chromium': True,
  'microsoft_use_internal_node': True,

  # Dependencies.
  # Chromium revision will be redefined in a gclient config because we use Chromium snapshots.
  'microsoft_chromium_version': '85.0.4183.121',
  'microsoft_depot_tools_version': '52fdd1ffcefb16435eee27023af95e5e844cfc16',
  'microsoft_electron_version': 'v10.4.5',
  'microsoft_node_version': 'v12.16.3',

  # Override defaults of the Electron checkout.
  'apply_patches': False,  # See comments below in the "hooks" section.
  'checkout_chromium': 'not microsoft_use_internal_chromium',
  'checkout_nan': False,
  'checkout_node': 'not microsoft_use_internal_node',
  'download_external_binaries': 'not microsoft_download_external_binaries',
}

deps = {
  'src': {
    'url': Var('microsoft_chromium_git') + '@' + Var('microsoft_chromium_version'),
    'condition': 'microsoft_use_internal_chromium',
  },

  'src/electron': {
    'url': Var('microsoft_electron_git') + '@' + Var('microsoft_electron_version'),
  },

  # We cannot easily use gclient's custom_deps, so we are going to checkout Node.js ourselves.
  'src/third_party/electron_node': {
    'url': Var('microsoft_node_git') + '@' + Var('microsoft_node_version'),
    'condition': 'microsoft_use_internal_node',
  },
}

hooks = [
  # When Microsoft-internal versions of Chromium and Node.js are used
  # we have to make sure they match the Electron dependencies.
  {
    'name': 'check_chromium_version',
    'condition': 'microsoft_use_internal_chromium',
    'action': [
      'python',
      'src/microsoft/scripts/chromium_version.py',
      '--validate',
    ],
  },
  {
    'name': 'check_node_version',
    'condition': 'microsoft_use_internal_node',
    'action': [
      'python',
      'src/microsoft/scripts/node_version.py',
      '--validate',
    ],
  },

  # Backports from newer releases of Electron.
  {
    'name': 'backports_patches_for_electron',
    'action': [
      'python',
      'src/electron/script/apply_all_patches.py',
      'src/microsoft/patches/backports/electron/config.json',
    ],
  },

  # Apply Electron patches for Chromium and friends.
  {
    'name': 'upstream_patches',
    'condition': 'checkout_chromium or microsoft_use_internal_chromium',
    'action': [
      'python',
      'src/microsoft/scripts/apply_chromium_patches.py',
      '--to-a-chromium-snapshot={microsoft_use_internal_chromium}',
      'src/electron/patches/config.json',
    ],
  },

  # Backports from newer releases of Chromium.
  {
    'name': 'backports_patches_for_chromium',
    'condition': 'checkout_chromium or microsoft_use_internal_chromium',
    'action': [
      'python',
      'src/microsoft/scripts/apply_chromium_patches.py',
      '--to-a-chromium-snapshot={microsoft_use_internal_chromium}',
      'src/microsoft/patches/backports/chromium/config.json',
    ],
  },

  # We have to make a few changes just to make it possible to build Electron internally.
  {
    'name': 'internal_build_patches_for_chromium',
    'condition': 'checkout_chromium or microsoft_use_internal_chromium',
    'action': [
      'python',
      'src/microsoft/scripts/apply_chromium_patches.py',
      '--to-a-chromium-snapshot={microsoft_use_internal_chromium}',
      'src/microsoft/patches/internal_build/chromium/config.json',
    ],
  },
  {
    'name': 'internal_build_patches_for_electron',
    'action': [
      'python',
      'src/electron/script/apply_all_patches.py',
      'src/microsoft/patches/internal_build/electron/config.json',
    ],
  },
  {
    'name': 'internal_build_patches_for_electron_node',
    'action': [
      'python',
      'src/electron/script/apply_all_patches.py',
      'src/microsoft/patches/internal_build/electron_node/config.json',
    ],
  },

  # Add Microsoft-specific features.
  {
    'name': 'customization_patches_for_chromium',
    'condition': 'microsoft_apply_customization_patches and (checkout_chromium or microsoft_use_internal_chromium)',
    'action': [
      'python',
      'src/microsoft/scripts/apply_chromium_patches.py',
      '--to-a-chromium-snapshot={microsoft_use_internal_chromium}',
      'src/microsoft/patches/customization/chromium/config.json',
    ],
  },
  {
    'name': 'customization_patches_for_electron',
    'condition': 'microsoft_apply_customization_patches',
    'action': [
      'python',
      'src/electron/script/apply_all_patches.py',
      'src/microsoft/patches/customization/electron/config.json',
    ],
  },

  # Add custom build configs for FFmpeg.
  # It has be done after we apply patches because there are patches for FFmpeg
  # and those changes have to present in the custom configs.
  {
    'name': 'add_custom_ffmpeg_configs',
    'condition': 'microsoft_add_custom_ffmpeg_configs and (checkout_chromium or microsoft_use_internal_chromium)',
    'action': [
      'python',
      'src/microsoft/scripts/copy_ffmpeg_config.py',
      '--build-dir',
      'src/third_party/ffmpeg/chromium/config',
      '--config',
      'src/microsoft/configs/ffmpeg/microsoft.json',
      'src/microsoft/configs/ffmpeg/slim.json',
    ],
  },
  {
    'name': 'patch_custom_ffmpeg_configs',
    'condition': 'microsoft_add_custom_ffmpeg_configs and (checkout_chromium or microsoft_use_internal_chromium)',
    'action': [
      'python',
      'src/microsoft/scripts/apply_chromium_patches.py',
      '--to-a-chromium-snapshot={microsoft_use_internal_chromium}',
      'src/microsoft/patches/customization/ffmpeg/config.json',
    ],
  },

  # Install Linux sysroots manually if we use a Chromium snapshot.
  {
    'name': 'sysroot_arm',
    'pattern': '.',
    'condition': '(microsoft_use_internal_chromium and checkout_linux) and checkout_arm',
    'action': ['python', 'src/build/linux/sysroot_scripts/install-sysroot.py',
               '--arch=arm'],
  },
  {
    'name': 'sysroot_arm64',
    'pattern': '.',
    'condition': '(microsoft_use_internal_chromium and checkout_linux) and checkout_arm64',
    'action': ['python', 'src/build/linux/sysroot_scripts/install-sysroot.py',
               '--arch=arm64'],
  },
  {
    'name': 'sysroot_x64',
    'pattern': '.',
    'condition': '(microsoft_use_internal_chromium and checkout_linux) and checkout_x64',
    'action': ['python', 'src/build/linux/sysroot_scripts/install-sysroot.py',
               '--arch=x64'],
  },

  # Download build dependencies from an internal storage.
  {
    'name': 'internal_external_binaries',
    'pattern': 'src/electron/script/update-external-binaries.py',
    'condition': 'microsoft_download_external_binaries',
    'action': [
      'python',
      'src/electron/script/update-external-binaries.py',
      '--base-url',
      'https://dropsskypeelectron.blob.core.windows.net/external-binaries',
    ],
  },
]

recursedeps = [
  'src/electron',
]

# The gclient_args.gni file is required by gn.
# Here's a copy of the Electron config from its DEPS.
gclient_gn_args_file = 'src/build/config/gclient_args.gni'
gclient_gn_args = [
  'build_with_chromium',
  'checkout_android',
  'checkout_android_native_support',
  'checkout_google_benchmark',
  'checkout_libaom',
  'checkout_nacl',
  'checkout_oculus_sdk',
  'checkout_openxr',
  'checkout_pgo_profiles',
  'mac_xcode_version',
]
