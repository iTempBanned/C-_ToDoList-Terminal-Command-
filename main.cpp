#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include "json.hpp"

using json = nlohmann::json;

struct Task {
    int id;
    std::string description;
    bool completed;
    int priority; // 1=high, 2=medium, 3=low

    json to_json() const {
        return json{
            {"id", id},
            {"description", description},
            {"completed", completed},
            {"priority", priority}
        };
    }

    static Task from_json(const json& j) {
        return Task{
            j.at("id"),
            j.at("description"),
            j.at("completed"),
            j.at("priority")
        };
    }
};

class TaskManager {
private:
    std::vector<Task> tasks;
    std::string filename;
    int next_id;

public:
    TaskManager(const std::string& file) : filename(file), next_id(1) {
        load_tasks();
    }

    void add_task(const std::string& description, int priority = 2) {
        tasks.push_back({next_id++, description, false, priority});
        save_tasks();
        std::cout << "Task added with ID: " << (next_id - 1) << std::endl;
    }

    void list_tasks(bool show_completed = true) const {
        if (tasks.empty()) {
            std::cout << "No tasks found." << std::endl;
            return;
        }

        std::vector<Task> sorted_tasks = tasks;
        std::sort(sorted_tasks.begin(), sorted_tasks.end(), [](const Task& a, const Task& b) {
            if (a.priority != b.priority) return a.priority < b.priority;
            return a.id < b.id;
        });

        std::cout << "\nTASK LIST\n";
        std::cout << "---------\n";
        for (const auto& task : sorted_tasks) {
            if (!show_completed && task.completed) continue;

            std::string status = task.completed ? "[DONE]" : "[PENDING]";
            std::string priority_str;
            switch(task.priority) {
                case 1: priority_str = "HIGH"; break;
                case 2: priority_str = "MEDIUM"; break;
                case 3: priority_str = "LOW"; break;
                default: priority_str = "UNKNOWN";
            }

            std::cout << "ID: " << task.id
                      << " | " << status
                      << " | PRIORITY: " << priority_str
                      << " | " << task.description << std::endl;
        }
        std::cout << std::endl;
    }

    void mark_done(int id) {
        auto it = std::find_if(tasks.begin(), tasks.end(), [id](const Task& t) {
            return t.id == id;
        });

        if (it != tasks.end()) {
            it->completed = true;
            save_tasks();
            std::cout << "Task marked as completed!" << std::endl;
        } else {
            std::cout << "Task with ID " << id << " not found." << std::endl;
        }
    }

    void delete_task(int id) {
        auto it = std::remove_if(tasks.begin(), tasks.end(), [id](const Task& t) {
            return t.id == id;
        });

        if (it != tasks.end()) {
            tasks.erase(it, tasks.end());
            save_tasks();
            std::cout << "Task deleted successfully!!" << std::endl;
        } else {
            std::cout << "Task with ID " << id << " not found." << std::endl;
        }
    }

    void save_tasks() const {
        json j;
        for (const auto& task : tasks) {
            j.push_back(task.to_json());
        }

        std::ofstream file(filename);
        if (file.is_open()) {
            file << j.dump(4);
            file.close();
        } else {
            std::cerr << "Error saving tasks to file." << std::endl;
        }
    }

    void load_tasks() {
        std::ifstream file(filename);
        if (!file.is_open()) {
            save_tasks();
            return;
        }

        try {
            json j;
            file >> j;
            tasks.clear();

            for (const auto& item : j) {
                Task task = Task::from_json(item);
                tasks.push_back(task);
                if (task.id >= next_id) next_id = task.id + 1;
            }
        } catch (...) {
            std::cerr << "Error loading tasks from file. Starting with empty task list." << std::endl;
            tasks.clear();
            next_id = 1;
        }
    }
};

void show_help() {
    std::cout << "\nCOMMAND LINE TASK MANAGER\n";
    std::cout << "--------------------------\n";
    std::cout << "USAGE:\n";
    std::cout << "  add <description> [-p high|medium|low]  Add a new task\n";
    std::cout << "  list                                    List all tasks\n";
    std::cout << "  list pending                            List pending tasks only\n";
    std::cout << "  done <id>                               Mark task as completed\n";
    std::cout << "  delete <id>                             Delete a task\n";
    std::cout << "  help                                    Show this help message\n";
    std::cout << "  exit                                    Exit the program\n\n";
    std::cout << "EXAMPLES:\n";
    std::cout << "  add \"Buy groceries\" -p high\n";
    std::cout << "  add \"Walk the dog\"\n";
    std::cout << "  done 2\n\n";
}

int parse_priority(const std::string& priority_str) {
    if (priority_str == "high") return 1;
    if (priority_str == "low") return 3;
    return 2;
}

std::vector<std::string> split_command(const std::string& command) {
    std::vector<std::string> tokens;
    std::istringstream iss(command);
    std::string token;

    while (iss >> std::quoted(token)) {
        tokens.push_back(token);
    }

    return tokens;
}

int main() {
    TaskManager manager("tasks.json");
    show_help();

    std::string line;
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, line);

        if (line.empty()) continue;

        auto tokens = split_command(line);
        std::string command = tokens[0];

        if (command == "exit") {
            break;
        } else if (command == "help") {
            show_help();
        } else if (command == "add" && tokens.size() >= 2) {
            std::string description;
            int priority = 2;

            description = tokens[1];
            for (size_t i = 2; i < tokens.size(); ++i) {
                if (tokens[i] == "-p" && i+1 < tokens.size()) {
                    priority = parse_priority(tokens[++i]);
                } else {
                    description += " " + tokens[i];
                }
            }

            manager.add_task(description, priority);
        } else if (command == "list") {
            bool show_completed = true;
            if (tokens.size() > 1 && tokens[1] == "pending") {
                show_completed = false;
            }
            manager.list_tasks(show_completed);
        } else if (command == "done" && tokens.size() == 2) {
            try {
                int id = std::stoi(tokens[1]);
                manager.mark_done(id);
            } catch (...) {
                std::cout << "Invalid task ID." << std::endl;
            }
        } else if (command == "delete" && tokens.size() == 2) {
            try {
                int id = std::stoi(tokens[1]);
                manager.delete_task(id);
            } catch (...) {
                std::cout << "Invalid task ID." << std::endl;
            }
        } else {
            std::cout << "Unknown command. Type 'help' for available commands." << std::endl;
        }
    }

    std::cout << "Goodbye!" << std::endl;
    return 0;
}
