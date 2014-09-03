#include "RocketSystem.h"

#include <Rocket/Core.h>
#ifdef ROCKET_FREETYPE
    #include <Rocket/Core/FreeType/FontProvider.h>
#endif
#include <Rocket/Controls.h>
#include <Rocket/Debugger.h>
#include <QDir>
#include "ToolManager.h"
#include "LocalizationManagerInterface.h"
#include "ProjectManager.h"
#include "RocketFileInterface.h"
#include "Settings.h"

RocketSystem::RocketSystem() :
    renderInterface(),
    context( 0 ), context_w( 0 ), context_h( 0 ),
    eventListener( 0 )
{
    t.start();
}

RocketSystem::~RocketSystem()
{
    Rocket::Core::Shutdown();
}

float RocketSystem::GetElapsedTime()
{
    return t.elapsed();
}

int RocketSystem::TranslateString(Rocket::Core::String& translated, const Rocket::Core::String& input)
{
    QString
        translated_string,
        input_string;
    
    if(!LocalizationManagerInterface::hasInstance())
    {
        translated = input;
        return 0;
    }


    input_string = input.CString();

    if(input_string.contains(ProjectManager::getInstance().getLocalizationOpeningTag()))
    {
        QString localization_identifier;
        int starting_index = input_string.indexOf(ProjectManager::getInstance().getLocalizationOpeningTag()) + ProjectManager::getInstance().getLocalizationOpeningTag().size();
        
        
        int identifier_size = input_string.indexOf(ProjectManager::getInstance().getLocalizationClosingTag(),starting_index) - starting_index;

        localization_identifier = input_string.mid(starting_index, identifier_size);
        
        translated_string = LocalizationManagerInterface::getInstance().getLocalizationForIdentifier(localization_identifier.trimmed());
        
        translated_string = input_string.replace(ProjectManager::getInstance().getLocalizationOpeningTag()+localization_identifier+ProjectManager::getInstance().getLocalizationClosingTag(), translated_string);

        if(translated_string.isEmpty())
        {
            printf("warning: could not find localization identifier \"%s\"\n", localization_identifier.toLatin1().data() );
            translated = input;
            return 0;
        }
        else
        {
            translated = translated_string.toUtf8();
            return 1;
        }
    }
    else
    {
        translated = input;
        return 0;
    }

    
}

bool RocketSystem::initialize()
{
    Rocket::Core::SetRenderInterface(&renderInterface);
    Rocket::Core::SetSystemInterface(this);
    Rocket::Core::SetFileInterface(new RocketFileInterface());
    Rocket::Core::Initialise();

    #if defined ROCKET_FREETYPE || defined ROCKET_WITH_FREETYPE
        Rocket::Core::FreeType::FontProvider::Initialise();
    #else
        Rocket::Core::FontDatabase::Initialise();
    #endif

    Rocket::Controls::Initialise();

    context_w = Settings::getInt("ScreenSizeWidth", 1024);
    context_h = Settings::getInt("ScreenSizeHeight", 768);
    if (context_w && context_h)
        return createContext(context_w, context_h);
    else {
        context_w = 1024; context_h = 768;
        return createContext(1024, 768);
    }
}

void RocketSystem::finalize()
{
#ifdef ROCKET_FREETYPE
    Rocket::Core::FreeType::FontProvider::Shutdown();
#endif
    Rocket::Core::Shutdown();
    Rocket::Core::SetRenderInterface(0);
    Rocket::Core::SetSystemInterface(0);
    Rocket::Core::SetFileInterface(0);
}

bool RocketSystem::createContext(const int width, const int height)
{
    if (context)
        context->RemoveReference();

    context = Rocket::Core::CreateContext("main", Rocket::Core::Vector2i(width, height));

    if (context == NULL) 
    {
        Rocket::Core::Shutdown();
        return false;
    }

	Rocket::Debugger::Initialise(context);
    Rocket::Debugger::SetContext(context);

    context_w = width;
    context_h = height;

    eventListener = new EventListener();

    context->AddEventListener("click", eventListener, true);

    return true;
}

void RocketSystem::resizeContext(const int width, const int height)
{
    if (context == nullptr)
    {
        createContext(width, height);
        return;
    }

    context->SetDimensions(Rocket::Core::Vector2i(width, height));

    context_w = width;
    context_h = height;
}

void RocketSystem::loadFonts(const QString &directory_path)
{
    QDir directory(directory_path);
    QStringList name_filter_list;
    QStringList file_list;
    QString prefix;

    name_filter_list << "*.otf" << "*.ttf" << "*.fon" << "*.fnt" << "*.bmf";

    file_list = directory.entryList(name_filter_list);

    foreach(const QString &file_name, file_list) {
        loadFont(directory_path + file_name);
    }
}

void RocketSystem::loadFont(const QString &file)
{
    Rocket::Core::String
        r_string = file.toUtf8().data();

    #ifdef ROCKET_FREETYPE
        Rocket::Core::FreeType::FontProvider::LoadFontFace(r_string);
    #else
        Rocket::Core::FontDatabase::LoadFontFace(r_string);
    #endif
}

void RocketSystem::EventListener::ProcessEvent(Rocket::Core::Event &event)
{
    ToolManager::getInstance().getCurrentTool()->onElementClicked(event.GetTargetElement());
}


RocketSystem* RocketSystem::instance = 0;
