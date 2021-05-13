#pragma once

#include "common.h"
#include "VGMItem.h"

//typedef bool (*funcPtr)();
//typedef bool (VGMItem::*funcPtr)();

/*#define DECLARE_MENU_CALLS(menuclass, origclass)											\
	protected:																		\
		static menuclass<origclass> menu;											\
	public:																			\
	virtual vector<const char*>* GetMenuItemNames() {return menu.GetMenuItemNames();}	\
	virtual bool CallMenuItem(VGMItem* item, int menuItemNum){ return menu.CallMenuItem(item, menuItemNum); }
*/

#define BEGIN_MENU_SUB(origclass, parentclass)                                            \
    template <class T> class origclassMenu;                                                \
    protected:                                                                            \
        static origclassMenu<origclass> menu;                                            \
    public:                                                                                \
        std::vector<const wchar_t*>* GetMenuItemNames() override                            \
        {                                                                                \
            return menu.GetMenuItemNames();                                                \
        }                                                                                \
        bool CallMenuItem(VGMItem* item, int menuItemNum) override                        \
        {                                                                                \
            return menu.CallMenuItem(item, menuItemNum);                                \
        }                                                                                \
        template <class T> class origclassMenu : public parentclass::origclassMenu<T>    \
    {                                                                                    \
    public:                                                                                \
        origclassMenu()                                                                    \
        {


#define BEGIN_MENU(origclass)                                                            \
    template <class T> class origclassMenu;                                                \
    protected:                                                                            \
        static origclassMenu<origclass> menu;                                            \
    public:                                                                                \
        std::vector<const wchar_t*>* GetMenuItemNames() override                            \
        {                                                                                \
            return menu.GetMenuItemNames();                                                \
        }                                                                                \
        bool CallMenuItem(VGMItem* item, int menuItemNum) override                        \
        {                                                                                \
            return menu.CallMenuItem(item, menuItemNum);                                \
        }                                                                                \
    template <class T> class origclassMenu : public Menu<T>                                \
    {                                                                                    \
    public:                                                                                \
        origclassMenu()                                                                    \
        {

#define MENU_ITEM(origclass, func, menutext)    Menu<T>::AddMenuItem(&origclass::func, menutext);
#define END_MENU()    } };

#define DECLARE_MENU(origclass)   origclass::origclassMenu<origclass> origclass::menu;

//  ********
//  MenuItem
//  ********


/*class MenuItem
{
public:
	MenuItem( bool (VGMItem::*functionPtr)(), const char* theName, uint8_t theFlag = 0)
		: func(functionPtr), name(theName), flag(theFlag) {}
	~MenuItem() {}

public:
	const char* name;
	bool (*func)();
	uint8_t flag;
};*/

template<class T>
class Menu {
 public:
  Menu() = default;
  virtual ~Menu() = default;

  void AddMenuItem(bool (T::*funcPtr)(), const wchar_t *name, uint8_t flag = 0) {
    funcs.push_back(funcPtr);
    names.push_back(name);
    //menuItems.push_back(MenuItem(funcPtr, name, flag));
  }

  bool CallMenuItem(VGMItem *item, int menuItemNum) {
    return (dynamic_cast<T*>(item)->*funcs[menuItemNum])();
  }

  std::vector<const wchar_t *> *GetMenuItemNames() {
    return &names;
  }
  //vector<MenuItem>* GetMenuItems()
  //{
  //	return &menuItems;
  //}

 protected:
  std::vector<const wchar_t *> names;
  std::vector<bool (T::*)()> funcs;
  //vector<MenuItem> menuItems;
};