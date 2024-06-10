#pragma comment(lib,"D3D11.lib")
#pragma comment (lib,"Dxgi.lib")
#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_D3D11


#include <d3d11.h>
#include "openxr.h"
#include "openxr_platform.h"
#include "PositionData.hpp"
#include <iostream>
#include <vector>
#include <array>
#include <map>
#include <list>
#include <chrono>
#include "OscMessenger.hpp"




// extension functions - will be available after calling loadExtensionMethods
 PFN_xrEnumerateViveTrackerPathsHTCX xrEnumerateViveTrackerPathsHTCX = nullptr;
 PFN_xrGetD3D11GraphicsRequirementsKHR xrGetD3D11GraphicsRequirementsKHR = nullptr;

// InputState represents all trackers and their interaction capabillities
// Bindings between actions (get Location) and devices are created during setActions
// this is before the session is started and the connected trackers are enumerable
// Tracker paths and interactions are hard bound to their pose path
// in order to be able to detect trackers regardless of their role or to handle role changes
// paths and spaces for each possible pose
struct InputState {
  XrActionSet actionSet{XR_NULL_HANDLE};
  XrAction trackerPoseAction{XR_NULL_HANDLE};
  std::vector<XrPath> trackerPaths;
  std::vector<XrSpace> trackerSpaces;
};
struct Swapchain {
  XrSwapchain handle;
  int32_t width;
  int32_t height;
};
class XrProgram {
public:
XrProgram(OSCMessenger* _messenger){
  messenger = _messenger;
}
~XrProgram(){
  if (sessionRunning){
    xrEndSession(session);
  }
  xrDestroySpace(appSpace);
  for (auto space : input.trackerSpaces){
    xrDestroySpace(space);
  }
  xrDestroyAction(input.trackerPoseAction); 
  xrDestroyActionSet(input.actionSet);
  xrDestroySession(session);
  xrDestroyInstance(instance);
 
}

bool init(){
  result = enableExtensions();
  if(result != XR_SUCCESS){
    std::cout << "Error while enumerating extensions!" << '\n';
    return 1;
  }
  std::cout << "enumerated Extensions..." << '\n';
  result = createInstance();
  if (result != XR_SUCCESS){
    std::cout << "Error while creating Instance!" << '\n';
    return 1;
  }
  std::cout << "Instance created..." << '\n';
  result = getSystem();
  if (result != XR_SUCCESS){
    std::cout << "Error while creating system!" << '\n';
    return 1;
  }
  std::cout << "Syste created..." << '\n';

  result = createSession();
  if (result != XR_SUCCESS){
    std::cout << "Error while creating session!" << '\n';
    return 1;
  }
  std::cout << "Session created..." << '\n';
  result = setActions();
  if (result != XR_SUCCESS){
    std::cout << "Error while creating action set!" << '\n';
    return 1;
  }
  std::cout << "Action set attached..." << '\n';
  result = createReferenceSpace();
  if (result != XR_SUCCESS){
    std::cout << "Error while creating reference space!" << '\n';
    return 1;
  }
  std::cout << "Reference space created..." << '\n';
  result = createSwapchain();
  std::cout << "XrProgram initilisation successful..." << '\n';
  return 0;
}

XrResult enableExtensions(){
  // API Layers offer additional runtime-specific(?) functionalities.
  // These are available through provided extensions.
  // Used API Layers and extensions are passed as an array containing their names
  // For the sake of getting the vive trackers position only one extension is needed, which is part of the
  // openXR sdk base extensions, so there is no need to enable any API-Layers
  
  // Query Extensions
  uint32_t num_extensions;

  result = xrEnumerateInstanceExtensionProperties(nullptr, 0, &num_extensions, nullptr);
   if (result != XR_SUCCESS){
      return result;
    }
  extension_properties.resize(num_extensions);
  for(auto &ext : extension_properties){
    ext.type = XR_TYPE_EXTENSION_PROPERTIES;
  }
  result = xrEnumerateInstanceExtensionProperties(nullptr,(uint32_t) extension_properties.size(), &num_extensions, extension_properties.data());
  if (result != XR_SUCCESS){
      return result;
    }
  for (auto&  layer : api_layer_props){       
    api_layer_names.push_back(layer.layerName);
  }
   
  for (auto& ext: extension_properties){
    extension_names.push_back(ext.extensionName);
  }
  api_layer_names.clear();
  extension_names.clear();
  extension_names.push_back("XR_HTCX_vive_tracker_interaction");
  extension_names.push_back("XR_MND_headless");
  extension_names.push_back("XR_KHR_D3D11_enable");
  return result;
}

XrResult createInstance() {
  // Instance allows communication with runtime
  XrApplicationInfo appInfo = {
    .applicationName = "RecieveTrackerPositions",
    .applicationVersion = 1,
    .engineName = "No Engine",
    .engineVersion = 0,
    .apiVersion = XR_CURRENT_API_VERSION
  };

  // fill createInfo with data
  XrInstanceCreateInfo instanceCreateInfo = {
    .type = XR_TYPE_INSTANCE_CREATE_INFO,
    .next = nullptr,
    .applicationInfo = appInfo,
    .enabledApiLayerCount =static_cast<uint32_t>( api_layer_names.size()),
    .enabledApiLayerNames = api_layer_names.data(),
    .enabledExtensionCount =static_cast<uint32_t>( extension_names.size()),
    .enabledExtensionNames = extension_names.data()
  };

  result = xrCreateInstance(&instanceCreateInfo, &instance);
  if (result != XR_SUCCESS){
      return result;
  }
  // after the instance is created and the extensions from instanceCreateInfo are loaded
  // we can now activate the external functions declared earlier
  result = loadExtensionMethods();
  // check which runtime was found
  XrInstanceProperties instance_props = {
    .type = XR_TYPE_INSTANCE_PROPERTIES
  };
  result = xrGetInstanceProperties(instance, &instance_props);
  if (result == XR_SUCCESS){
    std::cout<< "Runtime: " << instance_props.runtimeName << '\n'; 
  }
  
  return result;
}

XrResult getSystem(){ 
  XrSystemGetInfo systemGetInfo = {
    .type = XR_TYPE_SYSTEM_GET_INFO,
    .formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY
  };
  result = xrGetSystem(instance, &systemGetInfo, &system_id);
  return result;
}

XrResult createSession(){
// session allow sync with the runtime by checking session state  and running the frame loop
// Even though no visual output is generated, providing a valid graphics api seems to be nessecary to
// get a proper synchronisation to the runtimes framerate.
// For this DirectX11 is choosen beacause it should work with most Windows maschines, as its part of windows
XrGraphicsRequirementsD3D11KHR requirement = {XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR};
result = xrGetD3D11GraphicsRequirementsKHR(instance, system_id, &requirement);
if (result != XR_SUCCESS){
  std::cout << "Error while getting DirectX11 Graphics Requirements" << '\n';
  return result;
}
bool r = d3d_init(requirement.adapterLuid);
if (r == false){
  std::cout <<"could not initialise DirectX" << '\n';
  return result;
}

XrGraphicsBindingD3D11KHR binding = {
  .type = XR_TYPE_GRAPHICS_BINDING_D3D11_KHR,
  .device = d3d11Device
};

  XrSessionCreateInfo session_create_info = {
    .type = XR_TYPE_SESSION_CREATE_INFO,
    .next = &binding,
    .systemId = system_id,

    
  };
  result = xrCreateSession(instance,&session_create_info ,&session);
  return result;
}

XrResult setActions() {

  // create ActionSet 
  XrActionSetCreateInfo actionSetInfo{XR_TYPE_ACTION_SET_CREATE_INFO};
  strcpy_s(actionSetInfo.actionSetName, "actions");
  strcpy_s(actionSetInfo.localizedActionSetName, "Actions");
  actionSetInfo.priority = 0;
  result = xrCreateActionSet(instance, &actionSetInfo, &input.actionSet );
  if (result !=XR_SUCCESS){
      return result;
  }
  // Create SubAction Paths
  // tracker
  input.trackerPaths.resize(13);
  result = xrStringToPath(instance, "/user/vive_tracker_htcx/role/handheld_object", &input.trackerPaths[0]);
  result = xrStringToPath(instance, "/user/vive_tracker_htcx/role/left_foot", &input.trackerPaths[1]);
  result = xrStringToPath(instance, "/user/vive_tracker_htcx/role/right_foot", &input.trackerPaths[2]);
  result = xrStringToPath(instance, "/user/vive_tracker_htcx/role/left_shoulder", &input.trackerPaths[3]);
  result = xrStringToPath(instance, "/user/vive_tracker_htcx/role/right_shoulder", &input.trackerPaths[4]);
  result = xrStringToPath(instance, "/user/vive_tracker_htcx/role/left_elbow", &input.trackerPaths[5]);
  result = xrStringToPath(instance, "/user/vive_tracker_htcx/role/right_elbow", &input.trackerPaths[6]);
  result = xrStringToPath(instance, "/user/vive_tracker_htcx/role/left_knee", &input.trackerPaths[7]);
  result = xrStringToPath(instance, "/user/vive_tracker_htcx/role/right_knee", &input.trackerPaths[8]);
  result = xrStringToPath(instance, "/user/vive_tracker_htcx/role/waist", &input.trackerPaths[9]);
  result = xrStringToPath(instance, "/user/vive_tracker_htcx/role/chest", &input.trackerPaths[10]);
  result = xrStringToPath(instance, "/user/vive_tracker_htcx/role/camera", &input.trackerPaths[11]);
  result = xrStringToPath(instance, "/user/vive_tracker_htcx/role/keyboard", &input.trackerPaths[12]);


  // create Actions
  XrActionCreateInfo actionInfo{XR_TYPE_ACTION_CREATE_INFO};
  actionInfo.actionType = XR_ACTION_TYPE_POSE_INPUT;

  //tracker
  strcpy_s(actionInfo.actionName, "tracker_pose");
  strcpy_s(actionInfo.localizedActionName," Tracker Pose");
  actionInfo.countSubactionPaths = uint32_t(input.trackerPaths.size());
  actionInfo.subactionPaths = input.trackerPaths.data();
  result = xrCreateAction(input.actionSet, &actionInfo, &input.trackerPoseAction);

  // Pose Paths
  // tracker 
  std::array<XrPath,13> trackerPosePaths;
  result = xrStringToPath(instance, "/user/vive_tracker_htcx/role/handheld_object/input/grip/pose", &trackerPosePaths[0]);
  result = xrStringToPath(instance, "/user/vive_tracker_htcx/role/left_foot/input/grip/pose", &trackerPosePaths[1]);
  result = xrStringToPath(instance, "/user/vive_tracker_htcx/role/right_foot/input/grip/pose", &trackerPosePaths[2]);
  result = xrStringToPath(instance, "/user/vive_tracker_htcx/role/left_shoulder/input/grip/pose", &trackerPosePaths[3]);
  result = xrStringToPath(instance, "/user/vive_tracker_htcx/role/right_shoulder/input/grip/pose", &trackerPosePaths[4]);
  result = xrStringToPath(instance, "/user/vive_tracker_htcx/role/left_elbow/input/grip/pose", &trackerPosePaths[5]);
  result = xrStringToPath(instance, "/user/vive_tracker_htcx/role/right_elbow/input/grip/pose", &trackerPosePaths[6]);
  result = xrStringToPath(instance, "/user/vive_tracker_htcx/role/left_knee/input/grip/pose", &trackerPosePaths[7]);
  result = xrStringToPath(instance, "/user/vive_tracker_htcx/role/right_knee/input/grip/pose", &trackerPosePaths[8]);
  result = xrStringToPath(instance, "/user/vive_tracker_htcx/role/waist/input/grip/pose", &trackerPosePaths[9]);
  result = xrStringToPath(instance, "/user/vive_tracker_htcx/role/chest/input/grip/pose", &trackerPosePaths[10]);
  result = xrStringToPath(instance, "/user/vive_tracker_htcx/role/camera/input/grip/pose", &trackerPosePaths[11]);
  result = xrStringToPath(instance, "/user/vive_tracker_htcx/role/keyboard/input/grip/pose", &trackerPosePaths[12]);

  result = xrStringToPath(instance, "/user/vive_tracker_htcx/role/waist/input/grip/pose", &trackerPosePaths[0]);

  // Bindings
  // create Paths for Binding
  XrPath viveTrackerInteractionProfilePath;
  result = xrStringToPath(instance, "/interaction_profiles/htc/vive_tracker_htcx", &viveTrackerInteractionProfilePath);

  // tracker
  std::vector<XrActionSuggestedBinding> trackerBindings{{
                                                      {input.trackerPoseAction, trackerPosePaths[0]},
                                                      {input.trackerPoseAction, trackerPosePaths[1]},
                                                      {input.trackerPoseAction, trackerPosePaths[2]},
                                                      {input.trackerPoseAction, trackerPosePaths[3]},
                                                      {input.trackerPoseAction, trackerPosePaths[4]},
                                                      {input.trackerPoseAction, trackerPosePaths[5]},
                                                      {input.trackerPoseAction, trackerPosePaths[6]},
                                                      {input.trackerPoseAction, trackerPosePaths[7]},
                                                      {input.trackerPoseAction, trackerPosePaths[8]},
                                                      {input.trackerPoseAction, trackerPosePaths[9]},
                                                      {input.trackerPoseAction, trackerPosePaths[10]},
                                                      {input.trackerPoseAction, trackerPosePaths[11]},
                                                      {input.trackerPoseAction, trackerPosePaths[12]} 
                                                        
                                                      }};
  XrInteractionProfileSuggestedBinding trackerSuggestedBindings{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
  trackerSuggestedBindings.interactionProfile = viveTrackerInteractionProfilePath;
  trackerSuggestedBindings.suggestedBindings = trackerBindings.data();
  trackerSuggestedBindings.countSuggestedBindings = (uint32_t) trackerBindings.size();
  result = xrSuggestInteractionProfileBindings(instance, &trackerSuggestedBindings);
  if (result == XR_SUCCESS){
    std::cout << "Suggested bindings accepted..." << '\n';
  }

  // Create Spaces
  XrActionSpaceCreateInfo actionSpaceInfo{XR_TYPE_ACTION_SPACE_CREATE_INFO};

  // tracker
   input.trackerSpaces.resize(13);
  actionSpaceInfo.action = input.trackerPoseAction;
  actionSpaceInfo.poseInActionSpace.orientation.w = 1.0f;
  actionSpaceInfo.subactionPath = input.trackerPaths[0];
  result = xrCreateActionSpace(session, &actionSpaceInfo, &input.trackerSpaces[0]);
  actionSpaceInfo.subactionPath = input.trackerPaths[1];
  result = xrCreateActionSpace(session, &actionSpaceInfo, &input.trackerSpaces[1]);
  actionSpaceInfo.subactionPath = input.trackerPaths[2];
  result = xrCreateActionSpace(session, &actionSpaceInfo, &input.trackerSpaces[2]);
  actionSpaceInfo.subactionPath = input.trackerPaths[3];
  result = xrCreateActionSpace(session, &actionSpaceInfo, &input.trackerSpaces[3]);
  actionSpaceInfo.subactionPath = input.trackerPaths[4];
  result = xrCreateActionSpace(session, &actionSpaceInfo, &input.trackerSpaces[4]);
  actionSpaceInfo.subactionPath = input.trackerPaths[5];
  result = xrCreateActionSpace(session, &actionSpaceInfo, &input.trackerSpaces[5]);
  actionSpaceInfo.subactionPath = input.trackerPaths[6];
  result = xrCreateActionSpace(session, &actionSpaceInfo, &input.trackerSpaces[6]);
  actionSpaceInfo.subactionPath = input.trackerPaths[7];
  result = xrCreateActionSpace(session, &actionSpaceInfo, &input.trackerSpaces[7]);
  actionSpaceInfo.subactionPath = input.trackerPaths[8];
  result = xrCreateActionSpace(session, &actionSpaceInfo, &input.trackerSpaces[8]);
  actionSpaceInfo.subactionPath = input.trackerPaths[9];
  result = xrCreateActionSpace(session, &actionSpaceInfo, &input.trackerSpaces[9]);
  actionSpaceInfo.subactionPath = input.trackerPaths[10];
  result = xrCreateActionSpace(session, &actionSpaceInfo, &input.trackerSpaces[10]);
  actionSpaceInfo.subactionPath = input.trackerPaths[11];
  result = xrCreateActionSpace(session, &actionSpaceInfo, &input.trackerSpaces[11]);
  actionSpaceInfo.subactionPath = input.trackerPaths[12];
  result = xrCreateActionSpace(session, &actionSpaceInfo, &input.trackerSpaces[12]);
  

  // Attach Action Set to Session
  XrSessionActionSetsAttachInfo attachInfo{XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};
  attachInfo.countActionSets = 1;
  attachInfo.actionSets = &input.actionSet;
  result = xrAttachSessionActionSets(session, &attachInfo);

  return result;
}

XrResult createReferenceSpace(){

  XrPosef positionInSpace{{0.0f,0.0f,0.0f,1.0f}, {0.0f,0.0f,0.0f}};
  XrReferenceSpaceCreateInfo referenceSpaceInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
  referenceSpaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
  referenceSpaceInfo.poseInReferenceSpace = positionInSpace;

  result = xrCreateReferenceSpace(session, &referenceSpaceInfo, &appSpace);
  if (result != XR_SUCCESS){
    return result;
  }
  XrExtent2Df bounds;
  result = xrGetReferenceSpaceBoundsRect(session, XR_REFERENCE_SPACE_TYPE_STAGE, &bounds);
  std::cout << "Reference space dimensions w: " << bounds.width << " h: " << bounds.height << '\n';
  return result;
}

XrResult createSwapchain(){
  XrSystemProperties systemProps {XR_TYPE_SYSTEM_PROPERTIES};
  result = xrGetSystemProperties(instance, system_id, &systemProps);
  uint32_t viewCount;
  result = xrEnumerateViewConfigurationViews(instance, system_id, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0, &viewCount, nullptr);
  configViews.resize(viewCount, {XR_TYPE_VIEW_CONFIGURATION_VIEW});
  result = xrEnumerateViewConfigurationViews(instance, system_id, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, configViews.size(), &viewCount, configViews.data());
  uint32_t swapchainFormatCount;
  xrEnumerateSwapchainFormats(session, 0, &swapchainFormatCount, nullptr);
  std::vector<int64_t> swapchainFormats(swapchainFormatCount);
  xrEnumerateSwapchainFormats(session, swapchainFormats.size(), &swapchainFormatCount, swapchainFormats.data());
  colorSwapchainFormat =  29; // dont know if this works

  for (uint32_t i = 0; i < viewCount; i++){
    const XrViewConfigurationView& vp = configViews[i];
    std::cout << "creating swapchain for view "<< i << " with w: "<<vp.recommendedImageRectWidth << " h: " << vp.recommendedImageRectHeight << " ss: " << vp.recommendedSwapchainSampleCount << '\n';
    XrSwapchainCreateInfo swapchainCreateInfo = {XR_TYPE_SWAPCHAIN_CREATE_INFO};
    swapchainCreateInfo.arraySize = 1;
    swapchainCreateInfo.format = colorSwapchainFormat;
    swapchainCreateInfo.width = vp.recommendedImageRectWidth;
    swapchainCreateInfo.height = vp.recommendedImageRectHeight;
    swapchainCreateInfo.mipCount = 1;
    swapchainCreateInfo.faceCount = 1;
    swapchainCreateInfo.sampleCount = vp.recommendedSwapchainSampleCount;
    swapchainCreateInfo.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;

    Swapchain swapchain;
    swapchain.width = swapchainCreateInfo.width;
    swapchain.height = swapchainCreateInfo.height;
    result= xrCreateSwapchain(session, &swapchainCreateInfo, &swapchain.handle);
    swapchains.push_back(swapchain);
  
  uint32_t imageCount;
  result = xrEnumerateSwapchainImages(swapchain.handle, 0, &imageCount, nullptr);
  
  // graphicsPlugin Code
  std::vector<XrSwapchainImageD3D11KHR> swapchainImageBuffer(imageCount, {XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR} );
  std::vector<XrSwapchainImageBaseHeader*> swapchainImages;
  for (XrSwapchainImageD3D11KHR& image : swapchainImageBuffer){
    swapchainImages.push_back(reinterpret_cast<XrSwapchainImageBaseHeader*>(&image) );
  }
m_swapchainImageBuffers.push_back(std::move(swapchainImageBuffer));
// ========================================
result = xrEnumerateSwapchainImages(swapchain.handle, imageCount, &imageCount, swapchainImages[0]);
m_swapchainImages.insert(std::make_pair(swapchain.handle, std::move(swapchainImages)));

  }



 return result;
}

XrResult pollEvents(){
 
while(tryReadNextEvent() != XR_EVENT_UNAVAILABLE){
 
if (result == XR_SUCCESS){
  const XrActiveActionSet activeActionSet{input.actionSet, XR_NULL_PATH};
 
  if (eventDataBuffer.type == XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED){
    // start the Session
    sessionState = reinterpret_cast<XrEventDataSessionStateChanged&>(eventDataBuffer).state;
    if (sessionState == XR_SESSION_STATE_READY){
        std::cout << "READY" << '\n';
     XrSessionBeginInfo sessionBeginInfo{
      .type = XR_TYPE_SESSION_BEGIN_INFO,
      .next = nullptr,
      .primaryViewConfigurationType =XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO
     };
     result = xrBeginSession(session, &sessionBeginInfo);
      if (result == XR_SUCCESS){
        sessionRunning = true;
        std::cout << "Session started..." << '\n';
      
      }
    }
    else {
      switch (sessionState)
      {
      case 0:
        std::cout << "UNKNOWN" << '\n';
        break;
      case 1:
        std::cout << "IDLE" << '\n';
        break;
        case 2:
        std::cout << "READY" << '\n';
        break;
        case 3:
        std::cout << "SYNCHRONIZED" << '\n';
        break;
        case 4:
        std::cout << "VISIBLE" << '\n';
        break;
        case 5:
        std::cout << "FOCUSED" << '\n';
        break;
        case 6:
        std::cout << "STOPPING" << '\n';
        break;
        case 7:
        std::cout << "LOSS PENDING" << '\n';
        break;
        case 8:
        std::cout << "EXTING" << '\n';
        break;
      default:
        break;
      }
      
    }
  }
  // connected Tracker changes
  if (eventDataBuffer.type == XR_TYPE_EVENT_DATA_VIVE_TRACKER_CONNECTED_HTCX){
    std::cout << "connections changed" << '\n';
    getActiveTrackers();
    int t = 0;
   
  }
  if (eventDataBuffer.type == XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED){
  }
   XrActionsSyncInfo syncInfo{XR_TYPE_ACTIONS_SYNC_INFO};
  syncInfo.countActiveActionSets = 1;
  syncInfo.activeActionSets = &activeActionSet;
  result = xrSyncActions(session, &syncInfo);
  std::cout << "synced" << '\n';
  // to transition to the right session state (focused) in which position data can be obtained
  // the headset must have been seen bz the basestation at least once
  if ((result != XR_SUCCESS)&&headsetWasSeen == false) {
    std::cout << "cant sync" << '\n'; 
    std::cout << "Headset must be visible at least once! "<< '\n';
  }
  if ((result == XR_SUCCESS)&&headsetWasSeen == false){
    headsetWasSeen = true;
  }
 
} else {
 /*  std::cout << "Event not valid" << '\n'; */
}
}
 return result;

}

void getPositionData(){
  auto time = std::chrono::system_clock::now();
  // get Positions
  XrActionStateGetInfo getInfo{XR_TYPE_ACTION_STATE_GET_INFO};
  XrActionStatePose poseState{XR_TYPE_ACTION_STATE_POSE};
  XrSpaceLocation spaceLocation{XR_TYPE_SPACE_LOCATION};

   // waitFrame blocks until the runtime asks for another frame to be rendered and thus keeps sync
 // return a frameState object with a predicted time, which can be used to locate the poition of the tracker. 
  XrFrameWaitInfo waitInfo {XR_TYPE_FRAME_WAIT_INFO};
  XrFrameState frameState{XR_TYPE_FRAME_STATE};
  result = xrWaitFrame(session, &waitInfo,  &frameState);
  
// iterate over connected tracker
for (int i = 0 ; i < connectedTrackers.size(); i++){
      // get the position of the tracker as quaternion
  
    result = xrLocateSpace(input.trackerSpaces[connectedTrackers[i]], appSpace, frameState.predictedDisplayTime, &spaceLocation);
    // check if was able to get a position and if its valid
    if (result == XR_SUCCESS){
      if (spaceLocation.locationFlags == 15){
        // first found tracker is defined to be the listener
        // any following are "speaker"
        QuaternionPosition position;
        if (0 == 0){
          position.sourceName = "listener";
        } else {
          position.sourceName = "speaker" + std::to_string(0);
        }
        position.posX = spaceLocation.pose.position.x;
        position.posY = spaceLocation.pose.position.y;
        position.posZ = spaceLocation.pose.position.z;
        position.oriW = spaceLocation.pose.orientation.w;
        position.oriX = spaceLocation.pose.orientation.x;
        position.oriY = spaceLocation.pose.orientation.y;
        position.oriZ = spaceLocation.pose.orientation.z;
        
        messenger->sendMessage(position);
      } else {
        std::cout << "position not valid" << '\n';
      }
    } else {
      char mssg[64];
      xrResultToString(instance, result, mssg);
      std:: cout << "Error locating space: " << mssg << '\n';
    }
}

XrFrameBeginInfo beginInfo{XR_TYPE_FRAME_BEGIN_INFO};
   auto t_beforeBegin =  std::chrono::duration_cast<std::chrono::duration<float>>( std::chrono::system_clock::now() - time).count() * 1000.0f;
  result = xrBeginFrame(session, &beginInfo);
 
  // end frame tells the runtime that the frame has finished
  // usually here the rendered frames should be submitted
  XrFrameEndInfo frameEndInfo{XR_TYPE_FRAME_END_INFO};
  frameEndInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
  frameEndInfo.displayTime = frameState.predictedDisplayTime;
  frameEndInfo.layerCount = 0;
  frameEndInfo.layers = nullptr;
  frameEndInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
  result =  xrEndFrame(session,& frameEndInfo);
  auto fullFrame =  std::chrono::duration_cast<std::chrono::duration<float>>( std::chrono::system_clock::now() - time).count() * 1000.0f;
  if (fullFrame > 15.0f ) {
    std::cout << "jitter, frame took  " << fullFrame <<" ms" <<'\n';
    int i = 0;
  }
}

void getActiveTrackers(){
  // checks which trackers are connected to the steamVR runtime and have a role != DEACTIVATED
  // and saves which action binding they correspond
  // this does not check if the tracker is powered on
  uint32_t numTrackerPaths = 0;
  std::vector<XrViveTrackerPathsHTCX> trackerPaths;
  result = xrEnumerateViveTrackerPathsHTCX(instance, 0, &numTrackerPaths, nullptr);
  trackerPaths.resize(numTrackerPaths);
  result = xrEnumerateViveTrackerPathsHTCX(instance, uint32_t(trackerPaths.size()), &numTrackerPaths, trackerPaths.data());
  std::cout << numTrackerPaths << " VIVE tracker detected" << '\n';
  connectedTrackers.clear();
   for (auto path : trackerPaths){
    uint32_t size;
    char role [XR_MAX_PATH_LENGTH];
    xrPathToString(instance,path.rolePath, sizeof(role), &size, role);
    std::cout << "- " << role << '\n';
    for (int i = 0; i < input.trackerPaths.size(); i++){
      if (input.trackerPaths[i] == path.rolePath){
       connectedTrackers.push_back(i);
      }
    }
   }
 }

XrResult tryReadNextEvent(){
  result = xrPollEvent(instance, &eventDataBuffer);
  return result;
}

XrResult loadExtensionMethods(){
  result = xrGetInstanceProcAddr(instance, "xrEnumerateViveTrackerPathsHTCX", (PFN_xrVoidFunction*)&xrEnumerateViveTrackerPathsHTCX);
  if (result != XR_SUCCESS){
    std::cout << "could not load extension method!" << '\n';
    return result;
  }
  result = xrGetInstanceProcAddr(instance, "xrGetD3D11GraphicsRequirementsKHR",(PFN_xrVoidFunction*)&xrGetD3D11GraphicsRequirementsKHR);
   if (result != XR_SUCCESS){
    std::cout << "could not load extension method!" << '\n';
    return result;
  }

  return result;
}

bool isSessionRunning(){
  return sessionRunning;
}

bool d3d_init(LUID &adapter_luid){
IDXGIAdapter1 *adapter = d3d_get_adapter(adapter_luid);
D3D_FEATURE_LEVEL featureLevels[] ={D3D_FEATURE_LEVEL_11_0};
if (adapter == nullptr){
  return false;
}
if(FAILED(D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, 0, 0, featureLevels, _countof(featureLevels), D3D11_SDK_VERSION, &d3d11Device, nullptr, &d3d_context ))) {
  return false;
}
adapter->Release();
return true;
}

IDXGIAdapter1 *d3d_get_adapter(LUID &adapter_luid){
  IDXGIAdapter1 *finalAdapter = nullptr;
  IDXGIAdapter1 *currentAdapter = nullptr;
  IDXGIFactory1 *dxgi_factoy;
  DXGI_ADAPTER_DESC1 adapter_desc;

  CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)(&dxgi_factoy));
  

  int curr = 0;
  while(dxgi_factoy->EnumAdapters1(curr++, &currentAdapter)== S_OK){
    currentAdapter->GetDesc1(&adapter_desc);

    if(memcmp(&adapter_desc.AdapterLuid, &adapter_luid, sizeof(&adapter_luid)) == 0){
      finalAdapter = currentAdapter;
      break;
    }
    currentAdapter->Release();
    currentAdapter = nullptr;
  }
  dxgi_factoy->Release();
  return finalAdapter;
}

XrSession session;
XrInstance instance;
XrSystemId system_id;
InputState input;
std::vector<XrExtensionProperties> extension_properties;
std::vector<const char*> extension_names;
std::vector<XrApiLayerProperties> api_layer_props;
std::vector<const char*> api_layer_names;
std::vector<XrViewConfigurationType> vcTypes;
XrEventDataBuffer eventDataBuffer{XR_TYPE_EVENT_DATA_BUFFER};
XrSessionState sessionState{XR_SESSION_STATE_UNKNOWN};
bool sessionRunning = false;
std::vector<int> connectedTrackers;
XrResult result;
XrSpace appSpace{XR_NULL_HANDLE};
bool headsetWasSeen = false;
bool isSynced = false;

std::vector<XrViewConfigurationView> configViews;
std::vector<Swapchain> swapchains;
std::map<XrSwapchain, std::vector<XrSwapchainImageBaseHeader*>> m_swapchainImages;
std::list<std::vector<XrSwapchainImageD3D11KHR>> m_swapchainImageBuffers;
std::vector<XrView> views;
int64_t colorSwapchainFormat{-1};


ID3D11Device *d3d11Device;
ID3D11DeviceContext *d3d_context;

OSCMessenger* messenger;

};