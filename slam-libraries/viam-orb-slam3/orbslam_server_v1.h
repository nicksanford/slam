// This is an experimental integration of orbslam into RDK.
#include <System.h>
#include <grpc/grpc.h>
#include <grpcpp/server.h>
#include <grpcpp/server_context.h>

#include <atomic>

#include "common/v1/common.grpc.pb.h"
#include "common/v1/common.pb.h"
#include "service/slam/v1/slam.grpc.pb.h"
#include "service/slam/v1/slam.pb.h"

using grpc::ServerContext;
using grpc::ServerWriter;
using viam::service::slam::v1::GetMapRequest;
using viam::service::slam::v1::GetMapResponse;
using viam::service::slam::v1::GetPointCloudMapRequest;
using viam::service::slam::v1::GetPointCloudMapResponse;
using viam::service::slam::v1::GetPositionNewRequest;
using viam::service::slam::v1::GetPositionNewResponse;
using viam::service::slam::v1::GetPositionRequest;
using viam::service::slam::v1::GetPositionResponse;
using viam::service::slam::v1::GetInternalStateRequest;
using viam::service::slam::v1::GetInternalStateResponse;
using viam::service::slam::v1::SLAMService;
using SlamPtr = std::unique_ptr<ORB_SLAM3::System>;

namespace viam {

static const int filenamePrefixLength = 6;
static const int checkForShutdownIntervalMicroseconds = 1e5;
extern std::atomic<bool> b_continue_session;

class SLAMServiceImpl final : public SLAMService::Service {
   public:
    ::grpc::Status GetPosition(ServerContext *context,
                               const GetPositionRequest *request,
                               GetPositionResponse *response) override;

    ::grpc::Status GetPositionNew(ServerContext *context,
                                  const GetPositionNewRequest *request,
                                  GetPositionNewResponse *response) override;

    ::grpc::Status GetMap(ServerContext *context, const GetMapRequest *request,
                          GetMapResponse *response) override;

    /*
      For a given GetPointCloudMapRequest
      Returns a GetPointCloudMapResponse containing a sparse
      slam map as Binary PCD

      Map uses z axis is in the direction the camera is facing
    */
    ::grpc::Status GetPointCloudMap(
        ServerContext *context, const GetPointCloudMapRequest *request,
        ServerWriter<GetPointCloudMapResponse>* response) override;

   ::grpc::Status GetInternalState(ServerContext *context,
                                       const GetInternalStateRequest *request,
                                       ServerWriter<GetInternalStateResponse>* response) override;

    void ProcessDataOnline(ORB_SLAM3::System *SLAM);

    void ProcessDataOffline(ORB_SLAM3::System *SLAM);

    // Creates a simple map containing a 2x4x8 rectangular prism with the robot
    // in the center, for testing GetMap and GetPosition.
    void ProcessDataForTesting(ORB_SLAM3::System *SLAM);

    void UpdateMapAndPose(ORB_SLAM3::System *SLAM, Sophus::SE3f tmpPose);

    void StartSaveAtlasAsOsa(ORB_SLAM3::System *SLAM);

    void StopSaveAtlasAsOsa();

    void SetSlam(ORB_SLAM3::System* s);
    bool ArchiveSlam(std::stringbuf &buffer);


    string path_to_data;
    string path_to_map;
    string path_to_sequence;
    string path_to_vocab;
    string path_to_settings;
    string slam_mode;
    string slam_port;
    string camera_name;
    chrono::milliseconds frame_delay_msec;
    chrono::seconds map_rate_sec;
    double yamlTime;
    std::atomic<bool> use_live_data{false};
    bool delete_processed_data = false;
    // The size of the buffer has to be the same as
    // dataBufferSize in RDK's builtin_test.go
    const int data_buffer_size = 4;
    int first_processed_file_index = -1;
    bool local_viewer_flag = false;
    bool pure_localization_mode = false;
    int n_key_frames = 0;
    int curr_map_id = 0;


   private:
    void SaveAtlasAsOsaWithTimestamp(ORB_SLAM3::System *SLAM);

    std::atomic<bool> finished_processing_offline{false};
    std::thread *thread_save_atlas_as_osa_with_timestamp;

    ORB_SLAM3::System* slam;
    std::mutex slam_mutex;
    Sophus::SE3f poseGrpc;
    std::vector<ORB_SLAM3::MapPoint *> currMapPoints;
};

namespace utils {

static const string time_format = "%Y-%m-%dT%H:%M:%S.0000Z";
enum class FileParserMethod { Recent, Closest };
// find a specific input argument from rdk and write the value to a string.
// Returns empty if the argument is not found.
string ArgParser(const vector<string> &args, const string varName);

// parse a config map for a specific variable name and return the value as a
// string. Returns empty if the variable is not found within the map.
string ConfigMapParser(string map, string varName);

// Parses and validates the command line arguments. Sets the log level. Throws
// an exception if the arguments are malformed.
void ParseAndValidateArguments(const vector<string> &args,
                               SLAMServiceImpl &slamService);

// Converts UTC time string to a double value.
double ReadTimeFromTimestamp(const string timestamp);

std::vector<std::string> ListFilesInDirectoryForCamera(
    const std::string data_directory, const std::string extension,
    const std::string camera_name);

// LoadRGB loads in rgb images to be used by ORBSLAM, and
// returns whether the image was loaded successfully
bool LoadRGB(std::string path_to_data, std::string filename, cv::Mat &imRGB);

// LoadRGBD loads in a rgbd pair of images to be used by ORBSLAM, and
// returns whether the current pair is okay
bool LoadRGBD(std::string path_to_data, std::string filename, cv::Mat &imRGB,
              cv::Mat &imDepth);

// Find the next frame based off the current interest given a directory of
// data and time to search from
int FindFrameIndex(const std::vector<std::string> &filesRGB,
                   std::string slam_mode, std::string path_to_data,
                   FileParserMethod interest, double configTime,
                   double *timeInterest);

// Make a filename to a specific location for a sensor with a timestamp
// currently does not support millisecond resolution
string MakeFilenameWithTimestamp(string path_to_dir, string camera_name);

// Removes data file
void RemoveFile(std::string file_path);

std::string PcdHeader(int mapSize);

void WriteFloatToBufferInBytes(std::string &buffer, float f);
void WriteFloatToBufferInBytes(std::vector<char> &buffer, float f);
void WriteEigenToBufferInBytes(std::vector<char> &buffer, Eigen::Matrix<float, 3, 1> v);

}  // namespace utils
}  // namespace viam
