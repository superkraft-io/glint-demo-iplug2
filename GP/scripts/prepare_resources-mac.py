#!/usr/bin/python3

# this script will create/update info plist files based on config.h and copy resources to the ~/Music/PLUG_NAME folder or the bundle depending on PLUG_SHARED_RESOURCES

kAudioUnitType_MusicDevice      = "aumu"
kAudioUnitType_MusicEffect      = "aumf"
kAudioUnitType_Effect           = "aufx"
kAudioUnitType_MIDIProcessor    = "aumi"

DONT_COPY = ("")

import plistlib, os, datetime, fileinput, glob, sys, string, shutil

scriptpath = os.path.dirname(os.path.realpath(__file__))
projectpath = os.path.abspath(os.path.join(scriptpath, os.pardir))

IPLUG2_ROOT = os.path.abspath(os.path.join(projectpath, "../third_party/iPlug2OOS/iPlug2"))

sys.path.insert(0, os.path.join(IPLUG2_ROOT, 'Scripts'))

from parse_config import parse_config, parse_xcconfig

def copy_resources_to_destination(projectpath, dst, label=""):
  """Copy image and font resources from project to destination folder."""
  display_dst = label if label else dst

  if os.path.exists(projectpath + "/resources/img/"):
    for img in os.listdir(projectpath + "/resources/img/"):
      print("copying " + img + " to " + display_dst)
      shutil.copy(projectpath + "/resources/img/" + img, dst)

  if os.path.exists(projectpath + "/resources/fonts/"):
    for font in os.listdir(projectpath + "/resources/fonts/"):
      print("copying " + font + " to " + display_dst)
      shutil.copy(projectpath + "/resources/fonts/" + font, dst)

def clean_config_string(value):
  return value.strip().strip('"').strip("'")

def main():
  config = parse_config(projectpath)
  xcconfig = parse_xcconfig(os.path.join(IPLUG2_ROOT, 'common-mac.xcconfig'))
  resource_base_name = os.environ.get("IPLUG_RESOURCE_BASENAME", config['BUNDLE_NAME'])
  skip_resource_copy = os.environ.get("IPLUG_SKIP_RESOURCE_COPY") == "1"

  plug_name = clean_config_string(config['PLUG_NAME'])
  plug_mfr = clean_config_string(config['PLUG_MFR'])
  bundle_name = clean_config_string(config['BUNDLE_NAME'])
  bundle_mfr = clean_config_string(config['BUNDLE_MFR'])
  bundle_domain = clean_config_string(config['BUNDLE_DOMAIN'])
  plug_copyright = clean_config_string(config['PLUG_COPYRIGHT_STR'])
  plug_unique_id = clean_config_string(config['PLUG_UNIQUE_ID'])
  plug_mfr_id = clean_config_string(config['PLUG_MFR_ID'])
  auv2_factory = clean_config_string(config['AUV2_FACTORY'])
  bundle_executable_name = bundle_name

  CFBundleGetInfoString = bundle_name + " v" + config['FULL_VER_STR'] + " " + plug_copyright
  CFBundleVersion = config['FULL_VER_STR']
  CFBundlePackageType = "BNDL"
  CSResourcesFileMapped = True
  LSMinimumSystemVersion = xcconfig['DEPLOYMENT_TARGET']

  if skip_resource_copy:
    print("Skipping resource copy ...")
  else:
    print("Copying resources ...")

    if config['PLUG_SHARED_RESOURCES']:
      dst = os.path.expanduser("~") + "/Music/" + bundle_name + "/Resources"
    else:
      dst = os.path.join(os.environ["TARGET_BUILD_DIR"], os.environ["UNLOCALIZED_RESOURCES_FOLDER_PATH"].lstrip('/'))

    if os.path.exists(dst) == False:
      os.makedirs(dst + "/", 0o0755 )

    copy_resources_to_destination(projectpath, dst)

    # Also copy resources to AUv3 Framework for macOS sandbox compatibility
    # (AUv3 appex cannot access container app's resources in sandbox)
    if not config['PLUG_SHARED_RESOURCES']:
      target_build_dir = os.environ.get("TARGET_BUILD_DIR", "")
      if target_build_dir:
        framework_dst = os.path.join(target_build_dir, bundle_name + ".app/Contents/Frameworks/AUv3Framework.framework/Versions/A/Resources")

        if os.path.exists(os.path.dirname(framework_dst)):
          if not os.path.exists(framework_dst):
            os.makedirs(framework_dst, 0o0755)

          copy_resources_to_destination(projectpath, framework_dst, "AUv3 Framework")

  print("Processing Info.plist files...")

# VST3

  plistpath = projectpath + "/resources/" + resource_base_name + "-VST3-Info.plist"
  with open(plistpath, 'rb') as fp:
    vst3 = plistlib.load(fp)
  vst3['CFBundleExecutable'] = bundle_executable_name
  vst3['CFBundleGetInfoString'] = CFBundleGetInfoString
  vst3['CFBundleIdentifier'] = bundle_domain + "." + bundle_mfr + ".vst3." + bundle_name + ""
  vst3['CFBundleName'] = bundle_name
  vst3['CFBundleVersion'] = CFBundleVersion
  vst3['CFBundleShortVersionString'] = CFBundleVersion
  vst3['LSMinimumSystemVersion'] = LSMinimumSystemVersion
  vst3['CFBundlePackageType'] = CFBundlePackageType
  vst3['CFBundleSignature'] = plug_unique_id
  vst3['CSResourcesFileMapped'] = CSResourcesFileMapped

  with open(plistpath, 'wb') as fp:
    plistlib.dump(vst3, fp)
# VST2

  plistpath = projectpath + "/resources/" + resource_base_name + "-VST2-Info.plist"
  with open(plistpath, 'rb') as fp:
    vst2 = plistlib.load(fp)
  vst2['CFBundleExecutable'] = bundle_executable_name
  vst2['CFBundleGetInfoString'] = CFBundleGetInfoString
  vst2['CFBundleIdentifier'] = bundle_domain + "." + bundle_mfr + ".vst." + bundle_name + ""
  vst2['CFBundleName'] = bundle_name
  vst2['CFBundleVersion'] = CFBundleVersion
  vst2['CFBundleShortVersionString'] = CFBundleVersion
  vst2['LSMinimumSystemVersion'] = LSMinimumSystemVersion
  vst2['CFBundlePackageType'] = CFBundlePackageType
  vst2['CFBundleSignature'] = plug_unique_id
  vst2['CSResourcesFileMapped'] = CSResourcesFileMapped

  with open(plistpath, 'wb') as fp:
    plistlib.dump(vst2, fp)

# CLAP

  plistpath = projectpath + "/resources/" + resource_base_name + "-CLAP-Info.plist"
  with open(plistpath, 'rb') as fp:
      clap = plistlib.load(fp)
  clap['CFBundleExecutable'] = bundle_executable_name
  clap['CFBundleGetInfoString'] = CFBundleGetInfoString
  clap['CFBundleIdentifier'] = bundle_domain + "." + bundle_mfr + ".clap." + bundle_name + ""
  clap['CFBundleName'] = bundle_name
  clap['CFBundleVersion'] = CFBundleVersion
  clap['CFBundleShortVersionString'] = CFBundleVersion
  clap['LSMinimumSystemVersion'] = LSMinimumSystemVersion
  clap['CFBundlePackageType'] = CFBundlePackageType
  clap['CFBundleSignature'] = plug_unique_id
  clap['CSResourcesFileMapped'] = CSResourcesFileMapped

  with open(plistpath, 'wb') as fp:
    plistlib.dump(clap, fp)

# AUDIOUNIT v2

  plistpath = projectpath + "/resources/" + resource_base_name + "-AU-Info.plist"
  with open(plistpath, 'rb') as fp:
    auv2 = plistlib.load(fp)
  auv2['CFBundleExecutable'] = bundle_executable_name
  auv2['CFBundleGetInfoString'] = CFBundleGetInfoString
  auv2['CFBundleIdentifier'] = bundle_domain + "." + bundle_mfr + ".audiounit." + bundle_name + ""
  auv2['CFBundleName'] = bundle_name
  auv2['CFBundleVersion'] = CFBundleVersion
  auv2['CFBundleShortVersionString'] = CFBundleVersion
  auv2['LSMinimumSystemVersion'] = LSMinimumSystemVersion
  auv2['CFBundlePackageType'] = CFBundlePackageType
  auv2['CFBundleSignature'] = plug_unique_id
  auv2['CSResourcesFileMapped'] = CSResourcesFileMapped

  if config['PLUG_TYPE'] == 0:
    if config['PLUG_DOES_MIDI_IN']:
      COMPONENT_TYPE = kAudioUnitType_MusicEffect
    else:
      COMPONENT_TYPE = kAudioUnitType_Effect
  elif config['PLUG_TYPE'] == 1:
    COMPONENT_TYPE = kAudioUnitType_MusicDevice
  elif config['PLUG_TYPE'] == 2:
    COMPONENT_TYPE = kAudioUnitType_MIDIProcessor

  auv2['AudioUnit Version'] = config['PLUG_VERSION_HEX']
  auv2['AudioComponents'] = [{}]
  auv2['AudioComponents'][0]['description'] = plug_name
  auv2['AudioComponents'][0]['factoryFunction'] = auv2_factory
  auv2['AudioComponents'][0]['manufacturer'] = plug_mfr_id
  auv2['AudioComponents'][0]['name'] = plug_mfr + ": " + plug_name
  auv2['AudioComponents'][0]['subtype'] = plug_unique_id
  auv2['AudioComponents'][0]['type'] = COMPONENT_TYPE
  auv2['AudioComponents'][0]['version'] = config['PLUG_VERSION_INT']
  auv2['AudioComponents'][0]['sandboxSafe'] = True

  with open(plistpath, 'wb') as fp:
    plistlib.dump(auv2, fp)
# AUDIOUNIT v3

  if config['PLUG_HAS_UI']:
    NSEXTENSIONPOINTIDENTIFIER  = "com.apple.AudioUnit-UI"
  else:
    NSEXTENSIONPOINTIDENTIFIER  = "com.apple.AudioUnit"

  plistpath = projectpath + "/resources/" + resource_base_name + "-macOS-AUv3-Info.plist"
  with open(plistpath, 'rb') as fp:
    auv3 = plistlib.load(fp)
  auv3['CFBundleExecutable'] = bundle_executable_name
  auv3['CFBundleGetInfoString'] = CFBundleGetInfoString
  auv3['CFBundleIdentifier'] = bundle_domain + "." + bundle_mfr + ".app." + bundle_name + ".AUv3"
  auv3['CFBundleName'] = bundle_name
  auv3['CFBundleVersion'] = CFBundleVersion
  auv3['CFBundleShortVersionString'] = CFBundleVersion
  auv3['LSMinimumSystemVersion'] = "10.14.0"
  auv3['CFBundlePackageType'] = "XPC!"
  auv3['NSExtension'] = dict(
  NSExtensionAttributes = dict(
                               AudioComponentBundle = bundle_domain + "." + bundle_mfr + ".app." + bundle_name + ".AUv3Framework",
                               AudioComponents = [{}]),
#                               NSExtensionServiceRoleType = "NSExtensionServiceRoleTypeEditor",
  NSExtensionPointIdentifier = NSEXTENSIONPOINTIDENTIFIER,
  NSExtensionPrincipalClass = "IPlugAUViewController_v" + resource_base_name
                             )
  auv3['NSExtension']['NSExtensionAttributes']['AudioComponents'] = [{}]
  auv3['NSExtension']['NSExtensionAttributes']['AudioComponents'][0]['description'] = plug_name
  auv3['NSExtension']['NSExtensionAttributes']['AudioComponents'][0]['manufacturer'] = plug_mfr_id
  auv3['NSExtension']['NSExtensionAttributes']['AudioComponents'][0]['name'] = plug_mfr + ": " + plug_name
  auv3['NSExtension']['NSExtensionAttributes']['AudioComponents'][0]['subtype'] = plug_unique_id
  auv3['NSExtension']['NSExtensionAttributes']['AudioComponents'][0]['type'] = COMPONENT_TYPE
  auv3['NSExtension']['NSExtensionAttributes']['AudioComponents'][0]['version'] = config['PLUG_VERSION_INT']
  auv3['NSExtension']['NSExtensionAttributes']['AudioComponents'][0]['sandboxSafe'] = True
  auv3['NSExtension']['NSExtensionAttributes']['AudioComponents'][0]['tags'] = [{}]

  if config['PLUG_TYPE'] == 1:
    auv3['NSExtension']['NSExtensionAttributes']['AudioComponents'][0]['tags'][0] = "Synth"
  else:
    auv3['NSExtension']['NSExtensionAttributes']['AudioComponents'][0]['tags'][0] = "Effects"

  with open(plistpath, 'wb') as fp:
    plistlib.dump(auv3, fp)
# AAX

  plistpath = projectpath + "/resources/" + resource_base_name + "-AAX-Info.plist"
  with open(plistpath, 'rb') as fp:
    aax = plistlib.load(fp)
  aax['CFBundleExecutable'] = bundle_executable_name
  aax['CFBundleGetInfoString'] = CFBundleGetInfoString
  aax['CFBundleIdentifier'] = bundle_domain + "." + bundle_mfr + ".aax." + bundle_name + ""
  aax['CFBundleName'] = bundle_name
  aax['CFBundleVersion'] = CFBundleVersion
  aax['CFBundleShortVersionString'] = CFBundleVersion
  aax['LSMinimumSystemVersion'] = LSMinimumSystemVersion
  aax['CSResourcesFileMapped'] = CSResourcesFileMapped

  with open(plistpath, 'wb') as fp:
    plistlib.dump(aax, fp)
# APP

  plistpath = projectpath + "/resources/" + resource_base_name + "-macOS-Info.plist"
  with open(plistpath, 'rb') as fp:
    macOSapp = plistlib.load(fp)
  macOSapp['CFBundleExecutable'] = bundle_executable_name
  macOSapp['CFBundleGetInfoString'] = CFBundleGetInfoString
  macOSapp['CFBundleIdentifier'] = bundle_domain + "." + bundle_mfr + ".app." + bundle_name + ""
  macOSapp['CFBundleName'] = bundle_name
  macOSapp['CFBundleVersion'] = CFBundleVersion
  macOSapp['CFBundleShortVersionString'] = CFBundleVersion
  macOSapp['LSMinimumSystemVersion'] = LSMinimumSystemVersion
  macOSapp['CFBundlePackageType'] = CFBundlePackageType
  macOSapp['CFBundleSignature'] = plug_unique_id
  macOSapp['CSResourcesFileMapped'] = CSResourcesFileMapped
  macOSapp['NSPrincipalClass'] = "SWELLApplication"
  macOSapp['NSMainNibFile'] = resource_base_name + "-macOS-MainMenu"
  macOSapp['LSApplicationCategoryType'] = "public.app-category.music"
  macOSapp['CFBundleIconFile'] = resource_base_name + ".icns"
  macOSapp['NSMicrophoneUsageDescription'] = 	"This app needs mic access to process audio."

  with open(plistpath, 'wb') as fp:
    plistlib.dump(macOSapp, fp)
if __name__ == '__main__':
  main()
