#ifndef RETRONODE_FILESYSTEM_PANEL_H
#define RETRONODE_FILESYSTEM_PANEL_H

#include <string>

namespace RetroNode {

class FileSystemPanel {
private:
    static void draw_directory_contents(const std::string& dir_path);

public:
    static void draw();
};

} // namespace RetroNode

#endif // RETRONODE_FILESYSTEM_PANEL_H
