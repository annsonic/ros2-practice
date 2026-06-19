#include <Arduino.h>
#include <WiFi.h>
#include <micro_ros_platformio.h>

#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <std_msgs/msg/string.h>
#include <string>

#include "secrets.h"


// =========================
// Wi-Fi configuration
// =========================
char kSsid[] = WIFI_SSID; 
char kPassword[] = WIFI_PASSWORD;

// =========================
// Network / Agent configuration
// =========================
IPAddress kAgentIP;
const uint16_t kAgentPort = static_cast<uint16_t>(strtoul(AGENT_PORT, nullptr, 10));
const char* kNodeName = "wifi_pubsub_node";

// =========================
// ROS topic configuration
// =========================
const char* kPublisherTopic = "micro_ros_response";
const char* kSubscriberTopic = "micro_ros_name";
const int kExecutorTimeoutMs = 100;
const size_t kDomainId = 8;

// =========================
// Connection diagnostics / debounce
// =========================
const uint32_t kPingCheckIntervalMs = 3000;   // check agent health every 3s (not every loop)
const uint8_t  kPingFailThreshold   = 3;      // require consecutive failures before disconnect

uint32_t g_last_ping_check_ms = 0;
uint8_t  g_ping_fail_count = 0;
bool     g_wifi_begin_called = false;

// =========================
// ROS entities
// =========================
rcl_allocator_t allocator;
rclc_support_t support;
rcl_node_t node;
rcl_publisher_t publisher;
rcl_subscription_t subscriber;
rclc_executor_t executor;

// =========================
// Message buffers
// =========================
std_msgs__msg__String received_msg;
std_msgs__msg__String response_msg;

char received_buffer[64];
char response_buffer[128];

// =========================
// Connection state machine
// =========================
enum class ConnectionState {
  kConnectingWiFi,
  kWaitingForAgent,
  kCreatingEntities,
  kConnected,
  kDisconnected
};

ConnectionState g_state = ConnectionState::kConnectingWiFi;

// Forward declarations
bool ConnectWiFi();
bool CreateEntities();
void DestroyEntities();
void HandleState();
void SubscriptionCallback(const void* msgin);
void PublishResponse();

// =========================
// Setup
// =========================
void setup() {
  Serial.begin(115200);
  delay(1000);

  if (!kAgentIP.fromString(AGENT_IP)) {   // micro-ROS Agent host IP
    Serial.println("[INIT] Invalid AGENT_IP in secrets.h");
  }

  Serial.println("\n[INIT] Starting micro-ROS Wi-Fi pub/sub...");

  // Initialize message structs with pre-allocated buffers
  received_msg.data.data = received_buffer;
  received_msg.data.capacity = sizeof(received_buffer);
  received_msg.data.size = 0;

  response_msg.data.data = response_buffer;
  response_msg.data.capacity = sizeof(response_buffer);
  response_msg.data.size = 0;
}

// =========================
// Main loop
// =========================
void loop() {
  HandleState();
  delay(50);
}

// =========================
// State handler
// =========================
void HandleState() {
  switch (g_state) {
    case ConnectionState::kConnectingWiFi: {
      if (ConnectWiFi()) {
        Serial.println("[WIFI] Connected");
        Serial.print("[WIFI] IP: ");
        Serial.println(WiFi.localIP());

        // Configure micro-ROS Wi-Fi transport after Wi-Fi is up
        set_microros_wifi_transports(kSsid, kPassword, kAgentIP, kAgentPort);

        g_state = ConnectionState::kWaitingForAgent;
      } else {
        Serial.println("[WIFI] Connect failed, retrying...");
        delay(1000);
      }
      break;
    }

    case ConnectionState::kWaitingForAgent: {
      // Ping agent: timeout 200ms, attempts 3
      if (RMW_RET_OK == rmw_uros_ping_agent(200, 3)) {
        Serial.println("[ROS] Agent reachable");
        g_state = ConnectionState::kCreatingEntities;
      } else {
        // keep waiting
        delay(200);
      }
      break;
    }

    case ConnectionState::kCreatingEntities: {
      if (CreateEntities()) {
        Serial.println("[ROS] Entities created. Connected!");
        g_ping_fail_count = 0;
        g_last_ping_check_ms = millis();
        g_state = ConnectionState::kConnected;
      } else {
        Serial.println("[ROS] Entity creation failed, retrying...");
        DestroyEntities();
        g_state = ConnectionState::kWaitingForAgent;
      }
      break;
    }

    case ConnectionState::kConnected: {
      // If Wi-Fi dropped, reset
      if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[WIFI] Lost connection");
        g_state = ConnectionState::kDisconnected;
        break;
      }

      // Process subscription callbacks first
      rclc_executor_spin_some(&executor, RCL_MS_TO_NS(kExecutorTimeoutMs));

      // Throttled health check (NOT every loop)
      uint32_t now = millis();
      if (now - g_last_ping_check_ms >= kPingCheckIntervalMs) {
        g_last_ping_check_ms = now;

        if (RMW_RET_OK == rmw_uros_ping_agent(200, 1)) {
          if (g_ping_fail_count > 0) {
            Serial.println("[ROS] Agent ping recovered");
          }
          g_ping_fail_count = 0;
        } else {
          g_ping_fail_count++;
          Serial.print("[ROS] Agent ping failed: ");
          Serial.print(g_ping_fail_count);
          Serial.print("/");
          Serial.println(kPingFailThreshold);

          if (g_ping_fail_count >= kPingFailThreshold) {
            Serial.println("[ROS] Agent disconnected (debounced)");
            g_state = ConnectionState::kDisconnected;
          }
        }
      }

      break;
    }

    case ConnectionState::kDisconnected: {
      DestroyEntities();
      Serial.println("[ROS] Cleaned up entities");

      g_ping_fail_count = 0;
      g_last_ping_check_ms = 0;

      // Reconnect Wi-Fi if needed; otherwise wait for agent again
      if (WiFi.status() != WL_CONNECTED) {
        g_wifi_begin_called = false;  // allow WiFi.begin() on next attempt
        g_state = ConnectionState::kConnectingWiFi;
      } else {
        g_state = ConnectionState::kWaitingForAgent;
      }
      break;
    }

    default:
      break;
  }
}

// =========================
// Wi-Fi connect helper
// =========================
bool ConnectWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    return true;
  }

  WiFi.mode(WIFI_STA);

  // Avoid calling WiFi.begin repeatedly in tight retries
  if (!g_wifi_begin_called) {
    WiFi.begin(kSsid, kPassword);
    g_wifi_begin_called = true;
  }

  Serial.print("[WIFI] Connecting");
  const uint32_t start = millis();
  const uint32_t timeout_ms = 15000;

  while (WiFi.status() != WL_CONNECTED && (millis() - start) < timeout_ms) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();

  return WiFi.status() == WL_CONNECTED;
}

// =========================
// ROS subscription callback
// =========================
void SubscriptionCallback(const void* msgin) {
  const std_msgs__msg__String* msg = (const std_msgs__msg__String*)msgin;
  if (msg == nullptr || msg->data.data == nullptr) {
    return;
  }

  Serial.print("[SUB] Received: ");
  Serial.println(msg->data.data);

  // Build response: "Hello <name>!"
  snprintf(response_buffer, sizeof(response_buffer), "Hello %s!", msg->data.data);
  response_msg.data.size = strlen(response_buffer);

  PublishResponse();
}

// =========================
// ROS publish helper
// =========================
void PublishResponse() {
  rcl_ret_t rc = rcl_publish(&publisher, &response_msg, NULL);
  if (rc == RCL_RET_OK) {
    Serial.print("[PUB] Published: ");
    Serial.println(response_msg.data.data);
  } else {
    Serial.print("[PUB] Publish failed, rc=");
    Serial.println((int)rc);
  }
}

// =========================
// Create ROS entities
// =========================
bool CreateEntities() {
  allocator = rcl_get_default_allocator();

  rcl_init_options_t init_options = rcl_get_zero_initialized_init_options();
  if (rcl_init_options_init(&init_options, allocator) != RCL_RET_OK) {
    return false;
  }

  if (rcl_init_options_set_domain_id(&init_options, kDomainId) != RCL_RET_OK) {
    rcl_init_options_fini(&init_options);
    return false;
  }

  if (rclc_support_init_with_options(&support, 0, NULL, &init_options, &allocator) != RCL_RET_OK) {
    rcl_init_options_fini(&init_options);
    return false;
  }

  if (rcl_init_options_fini(&init_options) != RCL_RET_OK) {
    return false;
  }

  if (rclc_node_init_default(&node, kNodeName, "", &support) != RCL_RET_OK) {
    return false;
  }

  if (rclc_publisher_init_default(
        &publisher,
        &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, String),
        kPublisherTopic) != RCL_RET_OK) {
    return false;
  }

  if (rclc_subscription_init_default(
        &subscriber,
        &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, String),
        kSubscriberTopic) != RCL_RET_OK) {
    return false;
  }

  // One handle: subscriber
  if (rclc_executor_init(&executor, &support.context, 1, &allocator) != RCL_RET_OK) {
    return false;
  }

  if (rclc_executor_add_subscription(
        &executor,
        &subscriber,
        &received_msg,
        &SubscriptionCallback,
        ON_NEW_DATA) != RCL_RET_OK) {
    return false;
  }

  return true;
}

// =========================
// Destroy ROS entities
// =========================
void DestroyEntities() {
  // Speed up cleanup if session is broken
  rmw_context_t* rmw_context = rcl_context_get_rmw_context(&support.context);
  if (rmw_context != nullptr) {
    (void)rmw_uros_set_context_entity_destroy_session_timeout(rmw_context, 0);
  }

  (void)rcl_subscription_fini(&subscriber, &node);
  (void)rcl_publisher_fini(&publisher, &node);
  (void)rclc_executor_fini(&executor);
  (void)rcl_node_fini(&node);
  (void)rclc_support_fini(&support);
}
