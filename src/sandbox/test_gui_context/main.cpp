#include "common/app.hpp"
#include "common/lever_gui.hpp"
#include "common/juice_pump_gui.hpp"
#include "common/lever_pull.hpp"
#include "common/common.hpp"
#include "common/juice_pump.hpp"
#include "common/random.hpp"
#include "training.hpp"
#include "nlohmann/json.hpp"
#include <imgui.h>

#ifdef _MSC_VER
#include <Windows.h>
#endif

#include <time.h>
#include <thread>
#include <iostream>
#include <fstream>

using json = nlohmann::json;

struct App;

void render_gui(App& app);
void task_update(App& app);
void setup(App& app);
void shutdown(App& app);

struct TrialRecord {
  double trial_start_time_stamp;  // time since the session starts
  int trial_number;
  int first_pull_id;
  int rewarded;
  int task_type; // indicate the task type and different cue color (maybe beep sounds too): 0 no reward; 1 - self; 2 - altruistic (not built yet); 3 - cooperative (not built yet); 4  - for training (one reward for each animal in the cue on period)
  float pulltime_thres; // the time window threshold for the cooperation pulling
};

struct BehaviorData {
  int trial_number;
  double time_points;
  int behavior_events;
};

struct SessionInfo {
  std::string lever1_animal;
  std::string lever2_animal;
  std::string experiment_date;
  float init_force;
  float high_force;
  int task_type;
  float pulltime_thres;
};

struct LeverReadout {
  int trial_number;
  double readout_timepoint;
  //float strain_gauge_lever1;
  //float strain_gauge_lever2;
  //float potentiometer_lever1;
  //float potentiometer_lever2;
  float strain_gauge_lever;
  float potentiometer_lever;
  int lever_id;
  int pull_or_release;

}; // under construction ... -WS


struct App : public om::App {
  ~App() override = default;
  void setup() override {
    ::setup(*this);
  }
  void gui_update() override {
    render_gui(*this);
  }
  void task_update() override {
    ::task_update(*this);
  }
  void shutdown() override {
    ::shutdown(*this);
  }
  
  // Variable initiation
  // Some of these variable can be changed accordingly for each session. - Weikang

  // file name

  std::string lever1_animal{ "Sparkle" };
  std::string lever2_animal{ "Artemis" };

  std::string experiment_date{ "20230117" };

  //std::string trialrecords_name = experiment_date + "_" + lever1_animal + "_" + lever2_animal + "_TrialRecord_1.json" ;
  //std::string bhvdata_name = experiment_date + "_" + lever1_animal + "_" + lever2_animal + "_bhv_data_1.json" ;
  //std::string sessioninfo_name = experiment_date + "_" + lever1_animal + "_" + lever2_animal + "_session_info_1.json";
  //std::string leverread_name = experiment_date + "_" + lever1_animal + "_" + lever2_animal + "_lever_reading_1.json";

  int tasktype{ 1 }; // indicate the task type and different cue color: 0 no reward; 1 - self; 2 - altruistic; 3 - cooperative; 4  - for training
  // int tasktype{rand()%2}; // indicate the task type and different cue color: 0 no reward; 1 - self; 2 - altruistic; 3 - cooperative; 4  - for training 
  // int tasktype{ rand()%4}; // indicate the task type and different cue color: 0 no reward; 1 - self; 2 - altruistic; 3 - cooperative; 4  - for training 

  // lever force setting condition
  bool allow_auto_lever_force_set{true}; // true, if use force as below; false, if manually select force level on the GUI. - WS
  float normalforce{ 100.0f }; // 130
  float releaseforce{ 350.0f }; // 350

  bool allow_automated_juice_delivery{false};

  int lever_force_limits[2]{0, 550};
  om::lever::PullDetect detect_pull[2]{};
  // float lever_position_limits[2]{25e3f, 33e3f};
  // float lever_position_limits[4]{ 64.5e3f, 65e3f, 14e2f, 55e2f}; // lever 1 and lever 2 have different potentiometer ranges - WS 
  // float lever_position_limits[4]{ 63.7e3f, 65.3e3f, 12e2f, 59e2f }; // lever 1 and lever 2 have different potentiometer ranges - WS 
  float lever_position_limits[4]{ 42.4e3f, 48.2e3f, 12.5e2f, 70e2f }; // lever 1 and lever 2 have different potentiometer ranges - WS 
  bool invert_lever_position[2]{true, false};
  
  //float new_delay_time{2.0f};
  double new_delay_time{om::urand()*4+3}; //random delay between 3 to 5 s (in unit of second)
  int juice_delay_time{ 1000 }; // from successful pulling to juice delivery (in unit of minisecond)

  // session threshold
  float new_total_time{ 3600.0f }; // the time for the session (in unit of second)
  // float new_total_time{ 15.0f }; // the time for the maximal trial time - only used for mutual cooperation condition (task type == 3)
  int total_trial_number{ 500 }; // the maximal trial number of a session

  // variables that are updated every trial
  int trialnumber{ 0 };
  int first_pull_id{ 0 };
  bool getreward[2]{false, false};
  int rewarded[2]{0, 0};
  om::TimePoint first_pull_time{}; 
  double other_pull_time{}; // time gap bwtween two pulls

  double timepoint{};
  om::TimePoint trialstart_time;
  double trial_start_time_forsave;
  om::TimePoint session_start_time;
  int behavior_event{}; // 0 - trial starts; 9 - trial ends; 1 - lever 1 is pulled; 2 - lever 2 is pulled; 3 - pump 1 delivery; 4 - pump 2 delivery; etc


  bool leverpulled[2]{ false, false };
  float leverpulledtime[2]{ 0,0 };  //mostly for the cooperative condition (taskytype = 3)
  float pulledtime_thres{ 1.0f }; // time difference that two animals has to pull the lever 

  om::lever::AutomatedPull automated_pull{};
  om::lever::AutomatedPullParams automated_pull_params{};
  bool need_trigger_automated_pull{};

  // initiate auditory cues
  std::optional<om::audio::BufferHandle> debug_audio_buffer;
  std::optional<om::audio::BufferHandle> start_trial_audio_buffer;
  std::optional<om::audio::BufferHandle> sucessful_pull_audio_buffer;
  std::optional<om::audio::BufferHandle> failed_pull_audio_buffer;

  // initiate stimuli if using colored squares
  om::Vec2f stim0_size{ 0.2f };
  om::Vec2f stim0_offset{ -0.4f, 0.25f };
  om::Vec3f stim0_color{ 1.0f };
  om::Vec3f stim0_color_noreward{ 0.5f, 0.5f, 0.5f };
  om::Vec3f stim0_color_cooper{ 1.0f, 1.0f, 0.0f };
  om::Vec3f stim0_color_disappear{ 0.0f };
  om::Vec2f stim1_size{ 0.2f };
  om::Vec2f stim1_offset{ 0.4f, 0.25f };
  om::Vec3f stim1_color{ 1.0f };
  om::Vec3f stim1_color_noreward{ 1.0f, 1.0f, 0.0f };
  om::Vec3f stim1_color_cooper{ 1.0f, 1.0f, 0.0f };
  om::Vec3f stim1_color_disappear{ 0.0f };
  // initiate stimuli if using other images (non colored squares)
  std::optional<om::gfx::TextureHandle> debug_image;

  // struct for saving data
  // std::ofstream save_trial_data_file;
  
  std::vector<TrialRecord> trial_records;
  std::vector<BehaviorData> behavior_data;
  std::vector<SessionInfo> session_info;
  std::vector<LeverReadout> lever_readout; // under construction

};

// save data for Trial Record
json to_json(const TrialRecord& trial) {
  json result;
  result["trial_number"] = trial.trial_number;
  result["rewarded"] = trial.rewarded;
  result["first_pull_id"] = trial.first_pull_id;
  result["task_type"] = trial.task_type;
  result["trial_starttime"] = trial.trial_start_time_stamp;
  result["pulltime_thres"] = trial.pulltime_thres;
  return result;
}

json to_json(const std::vector<TrialRecord>& records) {
  json result;
  for (auto& record : records) {
    result.push_back(to_json(record));
  }
  return result;
}


// save data for behavior data
json to_json(const BehaviorData& bhv_data) {
  json result;
  result["trial_number"] = bhv_data.trial_number;
  result["time_points"] = bhv_data.time_points;
  result["behavior_events"] = bhv_data.behavior_events;
  return result;
}

json to_json(const std::vector<BehaviorData>& time_stamps) {
  json result;
  for (auto& time_stamp : time_stamps) {
    result.push_back(to_json(time_stamp));
  }
  return result;
}


// save data for session information
json to_json(const SessionInfo& session_info) {
  json result;
  result["lever1_animal"] = session_info.lever1_animal;
  result["lever2_animal"] = session_info.lever2_animal;
  result["levers_initial_force"] = session_info.init_force;
  result["levers_increased_force"] = session_info.high_force;
  result["experiment_date"] = session_info.experiment_date;
  result["task_type"] = session_info.task_type;
  result["pulltime_thres"] = session_info.pulltime_thres;
  return result;
}

json to_json(const std::vector<SessionInfo>& session_info) {
  json result;
  for (auto& sessioninfos : session_info) {
    result.push_back(to_json(sessioninfos));
  }
  return result;
}

// save data for lever information
json to_json(const LeverReadout& lever_reading) {
  json result;
  result["trial_number"] = lever_reading.trial_number;
  result["readout_timepoint"] = lever_reading.readout_timepoint;
  //result["potentiometer_lever1"] = lever_reading.potentiometer_lever1;
  //result["potentiometer_lever2"] = lever_reading.potentiometer_lever2;
  //result["potentiometer_lever1"] = lever_reading.strain_gauge_lever1;
  //result["potentiometer_lever2"] = lever_reading.strain_gauge_lever1;
  result["potentiometer_lever2"] = lever_reading.potentiometer_lever;
  result["potentiometer_lever1"] = lever_reading.strain_gauge_lever;
  result["lever_id"] = lever_reading.lever_id;
  result["pull_or_release"] = lever_reading.pull_or_release;
  return result;
}

json to_json(const std::vector<LeverReadout>& lever_reads) {
  json result;
  for (auto& lever_read : lever_reads) {
    result.push_back(to_json(lever_read));
  }
  return result;
}


void setup(App& app) {

  
  //auto buff_p = std::string{OM_RES_DIR} + "/sounds/piano-c.wav";
  //app.debug_audio_buffer = om::audio::read_buffer(buff_p.c_str());
  if (app.tasktype == 0) {
    auto buff_p = std::string{ OM_RES_DIR } + "/sounds/start_trial_beep.wav";
    app.start_trial_audio_buffer = om::audio::read_buffer(buff_p.c_str());
    auto debug_image_p = std::string{ OM_RES_DIR } + "/images/calla_leaves.png";
    app.debug_image = om::gfx::read_2d_image(debug_image_p.c_str());
  }
  else if (app.tasktype == 1 || app.tasktype == 4) {
    auto buff_p = std::string{ OM_RES_DIR } + "/sounds/start_trial_beep.wav";
    app.start_trial_audio_buffer = om::audio::read_buffer(buff_p.c_str());
    // auto debug_image_p = std::string{ OM_RES_DIR } + "/images/calla_leaves.png";
    // app.debug_image = om::gfx::read_2d_image(debug_image_p.c_str());
  }
  else if (app.tasktype == 2) {
    auto buff_p = std::string{ OM_RES_DIR } + "/sounds/start_trial_beep.wav";
    app.start_trial_audio_buffer = om::audio::read_buffer(buff_p.c_str());
    auto debug_image_p = std::string{ OM_RES_DIR } + "/images/blue_triangle.png";
    app.debug_image = om::gfx::read_2d_image(debug_image_p.c_str());
  }
  else if (app.tasktype == 3) {
    auto buff_p = std::string{ OM_RES_DIR } + "/sounds/start_trial_beep.wav";
    app.start_trial_audio_buffer = om::audio::read_buffer(buff_p.c_str());
    auto debug_image_p = std::string{ OM_RES_DIR } + "/images/yellow_circle.png";
    app.debug_image = om::gfx::read_2d_image(debug_image_p.c_str());
  }
  auto buff_p1 = std::string{ OM_RES_DIR } + "/sounds/successful_beep.wav";
  app.sucessful_pull_audio_buffer = om::audio::read_buffer(buff_p1.c_str());

  auto buff_p2 = std::string{ OM_RES_DIR } + "/sounds/failed_beep.wav";
  app.failed_pull_audio_buffer = om::audio::read_buffer(buff_p2.c_str());

  // define the threshold of pulling
  const float dflt_rising_edge = 0.5f;  // 0.6f
  const float dflt_falling_edge = 0.2f; // 0.25f
  app.detect_pull[0].rising_edge = dflt_rising_edge;
  app.detect_pull[1].rising_edge = dflt_rising_edge;
  app.detect_pull[0].falling_edge = dflt_falling_edge;
  app.detect_pull[1].falling_edge = dflt_falling_edge;

  // initialize lever force
  if (app.allow_auto_lever_force_set) {
    om::lever::set_force(om::lever::get_global_lever_system(), app.levers[0], app.normalforce);
    om::lever::set_force(om::lever::get_global_lever_system(), app.levers[1], app.normalforce);
  }

 
}

void shutdown(App& app) {
  (void)app;

  std::string trialrecords_name = app.experiment_date + "_" + app.lever1_animal + "_" + app.lever2_animal + "_TrialRecord_1.json";
  std::string bhvdata_name = app.experiment_date + "_" + app.lever1_animal + "_" + app.lever2_animal + "_bhv_data_1.json";
  std::string sessioninfo_name = app.experiment_date + "_" + app.lever1_animal + "_" + app.lever2_animal + "_session_info_1.json";
  std::string leverread_name = app.experiment_date + "_" + app.lever1_animal + "_" + app.lever2_animal + "_lever_reading_1.json";
 
  // check if the files already exit, if so, name them as xx_2.json
  std::string file_path0= std::string{ OM_DATA_DIR } +"/" + trialrecords_name;
  if (std::ifstream(file_path0))
  {
    trialrecords_name = app.experiment_date + "_" + app.lever1_animal + "_" + app.lever2_animal + "_TrialRecord_2.json";
    bhvdata_name = app.experiment_date + "_" + app.lever1_animal + "_" + app.lever2_animal + "_bhv_data_2.json";
    sessioninfo_name = app.experiment_date + "_" + app.lever1_animal + "_" + app.lever2_animal + "_session_info_2.json";
    leverread_name = app.experiment_date + "_" + app.lever1_animal + "_" + app.lever2_animal + "_lever_reading_2.json";
  }


  std::string file_path1 = std::string{ OM_DATA_DIR } +"/" + trialrecords_name;
  std::ofstream output_file1(file_path1);
  output_file1 << to_json(app.trial_records);

  std::string file_path2 = std::string{ OM_DATA_DIR } + "/" + bhvdata_name;
  std::ofstream output_file2(file_path2);
  output_file2 << to_json(app.behavior_data);

  // save some task information into session_info
  SessionInfo session_info{};
  session_info.lever1_animal = app.lever1_animal;
  session_info.lever2_animal = app.lever2_animal;
  session_info.high_force = app.releaseforce;
  session_info.init_force = app.normalforce;
  session_info.experiment_date = app.experiment_date;
  session_info.task_type = app.tasktype;
  session_info.pulltime_thres = app.pulledtime_thres;
  app.session_info.push_back(session_info);

  std::string file_path3 = std::string{ OM_DATA_DIR } + "/" + sessioninfo_name;
  std::ofstream output_file3(file_path3);
  output_file3 << to_json(app.session_info);

  std::string file_path4 = std::string{ OM_DATA_DIR } + "/" + leverread_name;
  std::ofstream output_file4(file_path4);
  output_file4 << to_json(app.lever_readout);
}


void render_lever_gui(App& app) {
  om::gui::LeverGUIParams gui_params{};
  gui_params.force_limit0 = app.lever_force_limits[0];
  gui_params.force_limit1 = app.lever_force_limits[1];
  gui_params.serial_ports = app.ports.data();
  gui_params.num_serial_ports = int(app.ports.size());
  gui_params.num_levers = int(app.levers.size());
  gui_params.levers = app.levers.data();
  gui_params.lever_system = om::lever::get_global_lever_system();
  auto gui_res = om::gui::render_lever_gui(gui_params);
  if (gui_res.force_limit0) {
    app.lever_force_limits[0] = gui_res.force_limit0.value();
  }
  if (gui_res.force_limit1) {
    app.lever_force_limits[1] = gui_res.force_limit1.value();
  }
}


void render_juice_pump_gui(App& app) {
  om::gui::JuicePumpGUIParams gui_params{};
  gui_params.serial_ports = app.ports.data();
  gui_params.num_ports = int(app.ports.size());
  gui_params.num_pumps = 2;
  gui_params.allow_automated_run = app.allow_automated_juice_delivery;
  auto res = om::gui::render_juice_pump_gui(gui_params);

  if (res.allow_automated_run) {
    app.allow_automated_juice_delivery = res.allow_automated_run.value();
  }
}

std::optional<std::string> render_text_input_field(const char* field_name) {
  const auto enter_flag = ImGuiInputTextFlags_EnterReturnsTrue;

  char text_buffer[50];
  memset(text_buffer, 0, 50);

  if (ImGui::InputText(field_name, text_buffer, 50, enter_flag)) {
    return std::string{ text_buffer };
  }
  else {
    return std::nullopt;
  }
}


void render_gui(App& app) {
  const auto enter_flag = ImGuiInputTextFlags_EnterReturnsTrue;



  ImGui::Begin("GUI");
  if (ImGui::Button("Refresh ports")) {
    app.ports = om::enumerate_ports();
  }

  if (ImGui::Button("start the trial")) {
    app.start_render = true;
  }

  if (ImGui::Button("pause the trial")) {
    app.start_render = false;
  }

  //if (auto m1_name = render_text_input_field("Lever1Animal")) {
  //  app.lever1_animal = m1_name.value();
  //}
  //if (auto m2_name = render_text_input_field("Lever2Animal")) {
  //  app.lever2_animal = m2_name.value();
  //}

  if (0) {
    int tasktype_gui[1]{ app.tasktype };
    if (ImGui::InputInt("Task_Type", tasktype_gui, 0, 0, enter_flag)) {
      app.tasktype = tasktype_gui[0];
    };

    float pulledtime_thres_gui[1]{ app.pulledtime_thres };
    if (ImGui::InputFloat("cooperation threshold (second)", pulledtime_thres_gui, 0.0f, 0.0f, "%0.1f", enter_flag)) {
      app.pulledtime_thres = pulledtime_thres_gui[0];
    };
  }

  render_lever_gui(app);

  

  if (ImGui::TreeNode("PullDetect")) {
    //ImGui::InputFloat2("PositionLimits", app.lever_position_limits);
    float lever_position_limits0[2]{app.lever_position_limits[0],app.lever_position_limits[1]};
    float lever_position_limits1[2]{app.lever_position_limits[2],app.lever_position_limits[3]};
    ImGui::InputFloat2("PositionLimits0", lever_position_limits0);
    ImGui::InputFloat2("PositionLimits1", lever_position_limits1);

    auto& detect = app.detect_pull;
    if (ImGui::InputFloat("RisingEdge", &detect[0].rising_edge, 0.0f, 0.0f, "%0.3f", enter_flag)) {
      detect[1].rising_edge = detect[0].rising_edge;
    }
    if (ImGui::InputFloat("FallingEdge", &detect[0].falling_edge, 0.0f, 0.0f, "%0.3f", enter_flag)) {
      detect[1].falling_edge = detect[1].rising_edge;
    }

    ImGui::TreePop();
  }


  if (ImGui::TreeNode("AutomatedPull")) {
    if (ImGui::Button("Trigger")) {
      app.need_trigger_automated_pull = true;
    }

    ImGui::TreePop();
  }

  if (ImGui::TreeNode("Stim0")) {
    ImGui::InputFloat3("Color", &app.stim0_color.x);
    ImGui::InputFloat2("Offset", &app.stim0_offset.x);
    ImGui::InputFloat2("Size", &app.stim0_size.x);
    ImGui::TreePop();
  }
  if (ImGui::TreeNode("Stim1")) {
    ImGui::InputFloat3("Color", &app.stim1_color.x);
    ImGui::InputFloat2("Offset", &app.stim1_offset.x);
    ImGui::InputFloat2("Size", &app.stim1_size.x);
    ImGui::TreePop();
  }
  
  ImGui::End();

  ImGui::Begin("JuicePump");
  render_juice_pump_gui(app);
  ImGui::End();
}

void update_automated_pull(App& app) {
  if (app.need_trigger_automated_pull && 
      app.automated_pull.state == om::lever::AutomatedPull::State::Idle) {
    om::lever::start_automated_pull(&app.automated_pull, app.normalforce);
    app.need_trigger_automated_pull = false;
  }

  auto* lever_sys = om::lever::get_global_lever_system();
  auto res = om::lever::update_automated_pull(&app.automated_pull, app.automated_pull_params);
  if (res.set_direction) {
    auto dir = res.set_direction.value() ?
      om::SerialLeverDirection::Forward : om::SerialLeverDirection::Reverse;
    om::lever::set_direction(lever_sys, app.levers[0], dir);
  }

  if (res.set_force) {
    om::lever::set_force(lever_sys, app.levers[0], res.set_force.value());
  }
}


float to_normalized(float v, float min, float max, bool inv) {
  if (min == max) {
    v = 0.0f;
  } else {
    v = om::clamp(v, min, max);
    v = (v - min) / (max - min);
  }
  return inv ? 1.0f - v : v;
}


void task_update(App& app) {
  using namespace om;

  static int state{};
  static bool entry{true};
  static NewTrialState new_trial{};
  static DelayState delay{};
  static InnerDelayState innerdelay{};
  static bool start_session_sound{true};



  //
  // renew for every new trial
  if (entry && state == 0) {
    
    // only use for trial-by-trial setting
    // app.tasktype = 1;
    // app.tasktype = rand()%2; // indicate the task type and different cue color (maybe beep sounds too): 0 no reward; 1 - self; 2 - altruistic (not built yet); 3 - cooperative (not built yet); 4  - for training (one reward for each animal in the cue on period)
    // app.tasktype = rand()%4; // indicate the task type and different cue color (maybe beep sounds too): 0 no reward; 1 - self; 2 - altruistic (not built yet); 3 - cooperative (not built yet); 4  - for training (one reward for each animal in the cue on period)
    //
    app.rewarded[0] = 0;
    app.rewarded[1] = 0;
    app.leverpulled[0] = false;
    app.leverpulled[1] = false;
    app.leverpulledtime[0] = 0;
    app.leverpulledtime[1] = 0;


    // sound to indicate the start of a session
    if (app.trialnumber == 0 && start_session_sound) {
      om::audio::play_buffer_both(app.start_trial_audio_buffer.value(), 0.5f);
      app.session_start_time = now();
      start_session_sound = false;

    }

    // end session when trialnumber or total sesison time reach the threshold
    //if (app.trialnumber > app.total_trial_number || elapsed_time(app.session_start_time, now()) > app.new_total_time) {
    //  abort;
    //}

    // push the lever force back to normal
    if (app.allow_auto_lever_force_set) {
      om::lever::set_force(om::lever::get_global_lever_system(), app.levers[0], app.normalforce);
      om::lever::set_force(om::lever::get_global_lever_system(), app.levers[1], app.normalforce);
    }
  }

  // check the levers
  for (int i = 0; i < 2; i++) {
    const auto lh = app.levers[i];
    auto& pd = app.detect_pull[i];
    if (auto lever_state = om::lever::get_state(om::lever::get_global_lever_system(), lh)) {
      om::lever::PullDetectParams params{};
      params.current_position = to_normalized(
        lever_state.value().potentiometer_reading,
        app.lever_position_limits[2 * i],
        app.lever_position_limits[2 * i + 1],
        app.invert_lever_position[i]);
      auto pull_res = om::lever::detect_pull(&pd, params);
      // if (pull_res.pulled_lever && app.tasktype != 0) {
      // if (pull_res.pulled_lever && app.sucessful_pull_audio_buffer && state == 0) { // only pull during the trial, not the ITI (state == 1)  -WS
      if (pull_res.pulled_lever && app.sucessful_pull_audio_buffer) {

        if (app.tasktype != 3) {
          om::audio::play_buffer_on_channel(app.sucessful_pull_audio_buffer.value(), abs(i-1), 0.5f);
        }

        // trial starts # 1
        // trial starts whenever one of the animal pulls
        if (!app.leverpulled[0] && !app.leverpulled[1]) {
          app.trialnumber = app.trialnumber + 1;
          app.first_pull_id = i + 1;
          app.timepoint = 0;
          app.trialstart_time = now();
          app.trial_start_time_forsave = elapsed_time(app.session_start_time, now());
          app.first_pull_time = now();
          app.behavior_event = 0; // start of a trial
          BehaviorData time_stamps{};
          time_stamps.trial_number = app.trialnumber;
          time_stamps.time_points = app.timepoint;
          time_stamps.behavior_events = app.behavior_event;
          app.behavior_data.push_back(time_stamps);
        }

        // save some behavioral events data
        app.timepoint = elapsed_time(app.trialstart_time, now());
        app.behavior_event = i + 1; // lever i+1 (1 or 2) is pulled
        app.other_pull_time = elapsed_time(app.first_pull_time, now());
        BehaviorData time_stamps2{};
        time_stamps2.trial_number = app.trialnumber;
        time_stamps2.time_points = app.timepoint;
        time_stamps2.behavior_events = app.behavior_event;
        app.behavior_data.push_back(time_stamps2);

        // save some lever information data
        LeverReadout lever_read{};
        lever_read.trial_number = app.trialnumber;
        lever_read.readout_timepoint = app.timepoint;
        //lever_read.potentiometer_lever1 = lever_state.value().potentiometer_reading;
        //lever_read.strain_gauge_lever1 = lever_state.value().strain_gauge;
        //lever_read.potentiometer_lever2 = lever_state.value().potentiometer_reading;
        //lever_read.strain_gauge_lever2 = lever_state.value().strain_gauge;
        lever_read.strain_gauge_lever = lever_state.value().strain_gauge;
        lever_read.potentiometer_lever = lever_state.value().potentiometer_reading;
        lever_read.lever_id = i + 1;
        lever_read.pull_or_release = int(pull_res.pulled_lever);
        app.lever_readout.push_back(lever_read);

        app.leverpulled[i] = true;


        // high lever force to make the animal release the lever 
        if (app.allow_auto_lever_force_set) {
          om::lever::set_force(om::lever::get_global_lever_system(), lh, app.releaseforce);
        }

        // deliver juice accordingly
        // self condition
        if (app.tasktype == 1 || app.tasktype == 4) {
          auto pump_handle = om::pump::ith_pump(abs(i)); // pump id: 0 - pump 1; 1 - pump 2  -WS
          std::this_thread::sleep_for(std::chrono::milliseconds(app.juice_delay_time));
          om::pump::run_dispense_program(pump_handle);
          app.getreward[i] = true;
          app.rewarded[i] = 1;
          //
          app.timepoint = elapsed_time(app.trialstart_time, now());
          app.behavior_event = abs(i) + 3; // pump 1 or 2 deliver  
          BehaviorData time_stamps3{};
          time_stamps3.trial_number = app.trialnumber;
          time_stamps3.time_points = app.timepoint;
          time_stamps3.behavior_events = app.behavior_event;
          app.behavior_data.push_back(time_stamps3);
        }

        // altruistic condition
        else if (app.tasktype == 2) {
          auto pump_handle = om::pump::ith_pump(abs(i-1)); // pump id: 0 - pump 1; 1 - pump 2  -WS
          std::this_thread::sleep_for(std::chrono::milliseconds(app.juice_delay_time));
          om::pump::run_dispense_program(pump_handle);
          app.getreward[abs(i - 1)] = true;
          app.rewarded[abs(i - 1)] = 1;
          //
          app.timepoint = elapsed_time(app.trialstart_time, now());
          app.behavior_event = abs(i-1) + 3; // pump 1 or 2 deliver  
          BehaviorData time_stamps3{};
          time_stamps3.trial_number = app.trialnumber;
          time_stamps3.time_points = app.timepoint;
          time_stamps3.behavior_events = app.behavior_event;
          app.behavior_data.push_back(time_stamps3);
        }

        // mutual cooperative condition (see below)
        // examine the other animal to determine how the trial ends 
        if (lever_read.lever_id == abs(app.first_pull_id - 2) + 1) {
          if (app.other_pull_time < app.pulledtime_thres) {

            // cooperative condition
            if (app.tasktype == 3) {
              if (app.leverpulled[0] && app.leverpulled[1]) {

                om::audio::play_buffer_both(app.sucessful_pull_audio_buffer.value(), 0.5f);

                // pump 0
                auto pump_handle = om::pump::ith_pump(0); // pump id: 0 - pump 1; 1 - pump 2  -WS
                std::this_thread::sleep_for(std::chrono::milliseconds(app.juice_delay_time));
                om::pump::run_dispense_program(pump_handle);
                app.getreward[0] = true;
                app.rewarded[0] = 1;
                //
                app.timepoint = elapsed_time(app.trialstart_time, now());
                app.behavior_event = 0 + 3; // pump 1 or 2 deliver  
                BehaviorData time_stamps3{};
                time_stamps3.trial_number = app.trialnumber;
                time_stamps3.time_points = app.timepoint;
                time_stamps3.behavior_events = app.behavior_event;
                app.behavior_data.push_back(time_stamps3);
                // pump 1
                auto pump_handle2 = om::pump::ith_pump(1); // pump id: 0 - pump 1; 1 - pump 2  -WS
                // std::this_thread::sleep_for(std::chrono::milliseconds(app.juice_delay_time));
                om::pump::run_dispense_program(pump_handle2);
                app.getreward[1] = true;
                app.rewarded[1] = 1;
                //
                app.timepoint = elapsed_time(app.trialstart_time, now());
                app.behavior_event = abs(1) + 3; // pump 1 or 2 deliver  
                BehaviorData time_stamps4{};
                time_stamps4.trial_number = app.trialnumber;
                time_stamps4.time_points = app.timepoint;
                time_stamps4.behavior_events = app.behavior_event;
                app.behavior_data.push_back(time_stamps4);
              }
            }
            state = 1;
            entry = true;
            break;
          }
          else {             
            // old edition
            // cooperative condition
            // if (app.tasktype == 3) {
            //   om::audio::play_buffer_both(app.failed_pull_audio_buffer.value(), 0.5f);
            // }

            //state = 1;
            //entry = true;
            //break;

            // new edition
            // end of a trial
            app.timepoint = elapsed_time(app.trialstart_time, now());
            app.behavior_event = 9; // end of a trial
            BehaviorData time_stamps{};
            time_stamps.trial_number = app.trialnumber;
            time_stamps.time_points = app.timepoint;
            time_stamps.behavior_events = app.behavior_event;
            app.behavior_data.push_back(time_stamps);
            //
            TrialRecord trial_record{};
            trial_record.trial_number = app.trialnumber;
            trial_record.first_pull_id = app.first_pull_id;
            trial_record.rewarded = app.rewarded[0] + app.rewarded[1];
            trial_record.task_type = app.tasktype;
            trial_record.pulltime_thres = app.pulledtime_thres;
            trial_record.trial_start_time_stamp = app.trial_start_time_forsave;
            //  Add to the array of trials.
            app.trial_records.push_back(trial_record);
            // 
            // 
            // trial starts #2
            app.trialnumber = app.trialnumber + 1;
            app.first_pull_id = i + 1;
            app.timepoint = 0;
            app.trialstart_time = now();
            app.trial_start_time_forsave = elapsed_time(app.session_start_time, now());
            app.first_pull_time = now();
            app.behavior_event = 0; // start of a trial
            BehaviorData time_stamps2{};
            time_stamps2.trial_number = app.trialnumber;
            time_stamps2.time_points = app.timepoint;
            time_stamps2.behavior_events = app.behavior_event;
            app.behavior_data.push_back(time_stamps2);
            //
            //app.timepoint = elapsed_time(app.trialstart_time, now());
            //app.behavior_event = i + 1; // lever i+1 (1 or 2) is pulled
            //app.other_pull_time = elapsed_time(app.first_pull_time, now());
            //BehaviorData time_stamps3{};
            //time_stamps3.trial_number = app.trialnumber;
            //time_stamps3.time_points = app.timepoint;
            //time_stamps3.behavior_events = app.behavior_event;
            //app.behavior_data.push_back(time_stamps3);



          }
        }
        else if (lever_read.lever_id == app.first_pull_id) {
          // if (app.other_pull_time >= app.pulledtime_thres) {
          //  state = 1;
          //  entry = true;
          //  break;
          //}
          
          // old edition
          app.first_pull_time = now();

          // new edition
          // trial starts #3
          //app.trialnumber = app.trialnumber + 1;
          //app.first_pull_id = i + 1;
          //app.timepoint = 0;
          //app.trialstart_time = now();
          //app.trial_start_time_forsave = elapsed_time(app.session_start_time, now());
          //app.first_pull_time = now();
          //app.behavior_event = 0; // start of a trial
          //BehaviorData time_stamps{};
          //time_stamps.trial_number = app.trialnumber;
          //time_stamps.time_points = app.timepoint;
          //time_stamps.behavior_events = app.behavior_event;
          //app.behavior_data.push_back(time_stamps);
         }
        
      }


      else if (pull_res.released_lever && app.allow_auto_lever_force_set) {
        om::lever::set_force(om::lever::get_global_lever_system(), lh, app.normalforce);

        // om::audio::play_buffer_both(app.failed_pull_audio_buffer.value(), 0.5f);
          
        // save some lever information data
        LeverReadout lever_read{};
        lever_read.trial_number = app.trialnumber;
        lever_read.readout_timepoint = elapsed_time(app.trialstart_time, now());;
        //lever_read.potentiometer_lever1 = lever_state.value().potentiometer_reading;
        //lever_read.strain_gauge_lever1 = lever_state.value().strain_gauge;
        //lever_read.potentiometer_lever2 = lever_state.value().potentiometer_reading;
        //lever_read.strain_gauge_lever2 = lever_state.value().strain_gauge;
        lever_read.strain_gauge_lever = lever_state.value().strain_gauge;
        lever_read.potentiometer_lever = lever_state.value().potentiometer_reading;
        lever_read.lever_id = i + 1;
        lever_read.pull_or_release = int(pull_res.pulled_lever);
        app.lever_readout.push_back(lever_read);

      }
    }
  }


  switch (state) {
    case 0: {
      // new_trial.play_sound_on_entry = app.start_trial_audio_buffer;
      new_trial.total_time = app.new_total_time;
      new_trial.stim0_offset = app.stim0_offset;
      new_trial.stim0_size = app.stim0_size;
      new_trial.stim1_offset = app.stim1_offset;
      new_trial.stim1_size = app.stim1_size;
      if (app.tasktype == 0) {
        //auto buff_p = std::string{ OM_RES_DIR } + "/sounds/start_trial_beep.wav";
        //app.start_trial_audio_buffer = om::audio::read_buffer(buff_p.c_str());
        // auto debug_image_p = std::string{ OM_RES_DIR } + "/images/calla_leaves.png";
        // app.debug_image = om::gfx::read_2d_image(debug_image_p.c_str());
        //
        new_trial.stim0_image = std::nullopt;
        new_trial.stim1_image = std::nullopt;
        new_trial.stim0_color = app.stim0_color_noreward;
        new_trial.stim1_color = app.stim1_color_noreward;
      } 
      else if (app.tasktype == 1) {
        //auto buff_p = std::string{ OM_RES_DIR } + "/sounds/start_trial_beep.wav";
        //app.start_trial_audio_buffer = om::audio::read_buffer(buff_p.c_str());
        //
        new_trial.stim0_image = std::nullopt;
        new_trial.stim1_image = std::nullopt;
        new_trial.stim0_color = app.stim0_color;
        new_trial.stim1_color = app.stim1_color;
      }
      else if (app.tasktype == 2) {
        //auto buff_p = std::string{ OM_RES_DIR } + "/sounds/start_trial_beep.wav";
        //app.start_trial_audio_buffer = om::audio::read_buffer(buff_p.c_str());
        //auto debug_image_p = std::string{ OM_RES_DIR } + "/images/blue_triangle.png";
       // app.debug_image = om::gfx::read_2d_image(debug_image_p.c_str());
        //
        new_trial.stim0_image = app.debug_image;
        new_trial.stim1_image = app.debug_image;
      }
      else if (app.tasktype == 3) {
        //auto buff_p = std::string{ OM_RES_DIR } + "/sounds/start_trial_beep.wav";
        //app.start_trial_audio_buffer = om::audio::read_buffer(buff_p.c_str());
        //auto debug_image_p = std::string{ OM_RES_DIR } + "/images/yellow_circle.png";
        //app.debug_image = om::gfx::read_2d_image(debug_image_p.c_str());
        //
        new_trial.stim0_image = app.debug_image;
        new_trial.stim1_image = app.debug_image;
      }
      else if (app.tasktype == 4) {
        //auto buff_p = std::string{ OM_RES_DIR } + "/sounds/start_trial_beep.wav";
        //app.start_trial_audio_buffer = om::audio::read_buffer(buff_p.c_str());
        //
        new_trial.stim0_image = std::nullopt;
        new_trial.stim1_image = std::nullopt;
        new_trial.stim0_color = app.stim0_color;
        new_trial.stim1_color = app.stim1_color;
        // if (app.leverpulled[0]) { new_trial.stim0_color = app.stim0_color_disappear; }
        // if (app.leverpulled[1]) { new_trial.stim1_color = app.stim1_color_disappear; }
        if (0) {
          //  Optionally specify an image handle - when this is set, the stim0_color parameter
          //  is ignored and the image is presented instead.
          // if (!app.leverpulled[0]) { new_trial.stim0_image = app.debug_image; }
          // else if (app.leverpulled[0]) { new_trial.stim0_image = {}; new_trial.stim0_color = app.stim0_color_disappear; }
          // if (!app.leverpulled[1]) { new_trial.stim1_image = app.debug_image; } //  works analogously for the other image. 
          // else if (app.leverpulled[1]) { new_trial.stim1_image = {}; new_trial.stim1_color = app.stim1_color_disappear; }  
        }
      }


      if (entry && app.allow_automated_juice_delivery) {
        auto pump_handle = om::pump::ith_pump(1); // pump id: 0 - pump 1; 1 - pump 2
        om::pump::run_dispense_program(pump_handle);
      }

      
      auto nt_res = tick_new_trial(&new_trial, &entry);
      if (nt_res.finished) {
        state = 1;
        entry = true;  
      }
      break;
    }

    case 1: {
        state = 2;
        entry = true;
        app.timepoint = elapsed_time(app.trialstart_time, now());
        app.behavior_event = 9; // end of a trial
        BehaviorData time_stamps{};
        time_stamps.trial_number = app.trialnumber;
        time_stamps.time_points = app.timepoint;
        time_stamps.behavior_events = app.behavior_event;
        app.behavior_data.push_back(time_stamps);
        app.getreward[0] = false;
        app.getreward[1] = false;
      break;
    }

    // save some data
    case 2: {

      TrialRecord trial_record{};
      trial_record.trial_number = app.trialnumber;
      trial_record.first_pull_id = app.first_pull_id;
      trial_record.rewarded = app.rewarded[0] + app.rewarded[1];
      trial_record.task_type = app.tasktype;
      trial_record.trial_start_time_stamp = app.trial_start_time_forsave;
      trial_record.pulltime_thres = app.pulledtime_thres;
      //  Add to the array of trials.
      app.trial_records.push_back(trial_record);
      state = 0;
      entry = true;
      break;
    }

    default: {
      assert(false);
    }
  }

#if 1
  update_automated_pull(app);
#endif
}



int main(int, char**) {
  srand(time(NULL));
  auto app = std::make_unique<App>();
  return app->run();

 
}

