#include "VVPlugin.h"

#include "VVController.h"

VVPlugin::VVPlugin(void) : Plugin(), m_host(), m_controller(nullptr) {}

VVPlugin::~VVPlugin(void) {
    delete m_controller;
}

QString VVPlugin::Name(void) {
    return "Verification && Validation";
}

QString VVPlugin::Description(void) {
    return "shows the panel for checking the geometry for errors";
}

void VVPlugin::Handle
(
    AppData& data
) {
    if (m_host.showPanel)
        m_host.showPanel();
}

void VVPlugin::SetHost
(
    const Host& host
) {
    m_host = host;

    delete m_controller;

    m_controller = new VVController(host);
}

QWidget* VVPlugin::CreatePanel(void) {
    return (m_controller != nullptr) ? m_controller->CreatePanel() : nullptr;
}

void VVPlugin::CreateMenu
(
    QMenuBar* menuBar
) {
    if (m_controller != nullptr)
        m_controller->CreateMenu(menuBar);
}

QList<QWidget*> VVPlugin::CreateToolbarButtons
(
    QWidget* parent
) {
    return (m_controller != nullptr) ? m_controller->CreateToolbarButtons(parent) : QList<QWidget*>();
}

void VVPlugin::Reset(void) {
    if (m_controller != nullptr)
        m_controller->Reset();
}
