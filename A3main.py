#!/usr/bin/env python3
import os
import sys
import ctypes
from ctypes import c_char_p, POINTER, c_int, c_void_p
import mysql.connector
from mysql.connector import Error
import datetime

from asciimatics.widgets import Frame, ListBox, Layout, Label, Divider, Text, Button, TextBox, PopUpDialog, Widget
from asciimatics.scene import Scene
from asciimatics.screen import Screen
from asciimatics.exceptions import ResizeScreenError, NextScene, StopApplication

# ------------------------
# Global DB Connection
# ------------------------
DB_CONN = None

# Global variable to store the absolute path to the main directory.
BASE_DIR = os.path.dirname(os.path.abspath(__file__))

# Global variable to hold the selected file path between scenes.
LAST_SELECTED_FILE = None

# ------------------------
# C Library Integration
# ------------------------

LIB_PATH = os.path.join(BASE_DIR, "bin", "libvcparser.so")
try:
    vc_parser = ctypes.CDLL(LIB_PATH)
except Exception as e:
    print("Error loading shared library:", e)
    sys.exit(1)

# Setup function prototypes
vc_parser.createCard.argtypes = [c_char_p, POINTER(c_void_p)]
vc_parser.createCard.restype = c_int

vc_parser.deleteCard.argtypes = [c_void_p]
vc_parser.deleteCard.restype = None

vc_parser.cardToString.argtypes = [c_void_p]
vc_parser.cardToString.restype = c_char_p

vc_parser.errorToString.argtypes = [c_int]
vc_parser.errorToString.restype = c_char_p

# Additional helper functions for Assignment 3.
vc_parser.createEmptyCard.argtypes = []
vc_parser.createEmptyCard.restype = c_void_p

vc_parser.updateFN.argtypes = [c_void_p, c_char_p]
vc_parser.updateFN.restype = c_int

# We'll assume OK is 0.
OK = 0

# ------------------------
# Database Helper Functions
# ------------------------

def init_db_tables(db_conn):
    """Creates the FILE and CONTACT tables if they do not already exist."""
    cursor = db_conn.cursor()
    cursor.execute("""
        CREATE TABLE IF NOT EXISTS FILE (
            file_id INT AUTO_INCREMENT PRIMARY KEY,
            file_name VARCHAR(60) NOT NULL,
            last_modified DATETIME,
            creation_time DATETIME NOT NULL
        )
    """)
    cursor.execute("""
        CREATE TABLE IF NOT EXISTS CONTACT (
            contact_id INT AUTO_INCREMENT PRIMARY KEY,
            name VARCHAR(256) NOT NULL,
            birthday DATETIME,
            anniversary DATETIME,
            file_id INT NOT NULL,
            FOREIGN KEY (file_id) REFERENCES FILE(file_id) ON DELETE CASCADE
        )
    """)
    db_conn.commit()
    cursor.close()

def extract_fn_from_card(file_path):
    """Creates a card using the C library and extracts the FN property."""
    card_ptr = c_void_p()
    ret = vc_parser.createCard(file_path.encode("utf-8"), ctypes.byref(card_ptr))
    if ret != OK:
        return None
    card_str_ptr = vc_parser.cardToString(card_ptr)
    card_str = ctypes.string_at(card_str_ptr).decode("utf-8")
    vc_parser.deleteCard(card_ptr)
    for line in card_str.splitlines():
        if line.startswith("FN:"):
            return line[3:].strip()
    return None

def update_db_with_card(file_path, db_conn, fn_value):
    """Inserts or updates the FILE and CONTACT records for the given file."""
    file_name = os.path.basename(file_path)
    mod_time = datetime.datetime.fromtimestamp(os.path.getmtime(file_path)).strftime('%Y-%m-%d %H:%M:%S')
    cursor = db_conn.cursor()
    cursor.execute("SELECT file_id, last_modified FROM FILE WHERE file_name = %s", (file_name,))
    result = cursor.fetchone()
    if result:
        file_id, last_modified = result
        if last_modified != mod_time:
            cursor.execute("UPDATE FILE SET last_modified = %s WHERE file_id = %s", (mod_time, file_id))
            db_conn.commit()
        cursor.execute("SELECT contact_id, name FROM CONTACT WHERE file_id = %s", (file_id,))
        contact = cursor.fetchone()
        if contact:
            contact_id, current_name = contact
            if current_name != fn_value:
                cursor.execute("UPDATE CONTACT SET name = %s WHERE contact_id = %s", (fn_value, contact_id))
                db_conn.commit()
    else:
        cursor.execute("INSERT INTO FILE (file_name, last_modified, creation_time) VALUES (%s, %s, %s)",
                       (file_name, mod_time, mod_time))
        db_conn.commit()
        file_id = cursor.lastrowid
        cursor.execute("INSERT INTO CONTACT (name, file_id) VALUES (%s, %s)", (fn_value, file_id))
        db_conn.commit()
    cursor.close()

def update_db_for_file(file_path, new_fn, db_conn):
    """Updates the CONTACT record for the given file with a new contact name."""
    file_name = os.path.basename(file_path)
    cursor = db_conn.cursor()
    cursor.execute("SELECT file_id FROM FILE WHERE file_name = %s", (file_name,))
    result = cursor.fetchone()
    if result:
        file_id = result[0]
        cursor.execute("UPDATE CONTACT SET name = %s WHERE file_id = %s", (new_fn, file_id))
        db_conn.commit()
    cursor.close()

def insert_db_for_new_file(file_path, contact, db_conn):
    """Inserts a new FILE and CONTACT record for a new vCard file."""
    update_db_with_card(file_path, db_conn, contact)

# ------------------------
# UI Classes (Asciimatics)
# ------------------------

class LoginFrame(Frame):
    def __init__(self, screen):
        super(LoginFrame, self).__init__(screen,
                                         screen.height,
                                         screen.width,
                                         can_scroll=False,
                                         title="DB Login")
        layout = Layout([100])
        self.add_layout(layout)
        self._username = Text("Username:", "username")
        self._password = Text("Password:", "password", hide_char="*")
        self._dbname   = Text("DB Name:", "dbname")
        layout.add_widget(self._username)
        layout.add_widget(self._password)
        layout.add_widget(self._dbname)
        layout.add_widget(Divider())
        layout.add_widget(Button("Login", self._login))
        layout.add_widget(Button("Cancel", self._cancel))
        self.fix()
        self.db_connection = None

    def _login(self):
        self.save()
        username = self.data["username"]
        password = self.data["password"]
        dbname   = self.data["dbname"]
        try:
            conn = mysql.connector.connect(
                host="dursley.socs.uoguelph.ca",
                user=username,
                password=password,
                database=dbname
            )
            if conn.is_connected():
                global DB_CONN
                DB_CONN = conn
                init_db_tables(DB_CONN)
                raise NextScene("Main")
        except Error as e:
            self.scene.add_effect(PopUpDialog(self.screen, "DB Connection Failed:\n" + str(e), ["OK"]))
    
    def _cancel(self):
        raise StopApplication("DB login cancelled by user.")

class MainFrame(Frame):
    def __init__(self, screen):
        super(MainFrame, self).__init__(screen,
                                        screen.height,
                                        screen.width,
                                        can_scroll=False,
                                        title="vCard List")
        self.db_conn = DB_CONN
        self.vcard_files = []
        layout = Layout([100])
        self.add_layout(layout)
        self._listbox = ListBox(
            Widget.FILL_FRAME,
            [],
            name="files",
            add_scroll_bar=True
        )
        layout.add_widget(Label("Select a valid vCard file:"))
        layout.add_widget(self._listbox)
        layout2 = Layout([1, 1, 1, 1])
        self.add_layout(layout2)
        layout2.add_widget(Button("View/Edit", self._view), 0)
        layout2.add_widget(Button("Create New", self._create), 1)
        layout2.add_widget(Button("DB View", self._dbview), 2)
        layout2.add_widget(Button("Exit", self._exit), 3)
        self.fix()
        self.refresh_file_list()

    def refresh_file_list(self):
        # Update the DB connection from the global variable.
        self.db_conn = DB_CONN
        if self.db_conn is None:
            self._listbox.options = [("No DB connection", None)]
            return

        self.vcard_files = []
        items = []
        # Use an absolute path to the "cards" folder in the main directory.
        cards_dir = os.path.join(BASE_DIR, "cards")
        if os.path.isdir(cards_dir):
            for f in os.listdir(cards_dir):
                if f.lower().endswith((".vcf", ".vcard")):
                    file_path = os.path.join(cards_dir, f)
                    card_ptr = c_void_p()
                    ret = vc_parser.createCard(file_path.encode("utf-8"), ctypes.byref(card_ptr))
                    if ret == OK:
                        fn_value = extract_fn_from_card(file_path)
                        if fn_value:
                            update_db_with_card(file_path, self.db_conn, fn_value)
                        self.vcard_files.append(file_path)
                        items.append((f, f))
                        vc_parser.deleteCard(card_ptr)
        self._listbox.options = items
        if items:
            self._listbox.value = items[0][1]
        else:
            self._listbox.options = [("No valid files", None)]
            self._listbox.value = None

    def _view(self):
        file_selected = self._listbox.value
        if file_selected:
            global LAST_SELECTED_FILE
            LAST_SELECTED_FILE = os.path.join(BASE_DIR, "cards", file_selected)
            raise NextScene("Detail")
        else:
            self.scene.add_effect(PopUpDialog(self.screen, "No file selected.", ["OK"]))
    
    def _create(self):
        raise NextScene("Create")
    
    def _dbview(self):
        raise NextScene("DB")
    
    def _exit(self):
        raise StopApplication("User requested exit.")

class DetailFrame(Frame):
    def __init__(self, screen):
        super(DetailFrame, self).__init__(screen,
                                          screen.height,
                                          screen.width,
                                          can_scroll=False,
                                          title="Card Details")
        layout = Layout([100])
        self.add_layout(layout)
        self._file_label = Label("File: ")
        self._contact = Text("Contact:", "contact")
        self._details = TextBox(10, "Details", readonly=True)
        layout.add_widget(self._file_label)
        layout.add_widget(self._contact)
        layout.add_widget(Divider())
        layout.add_widget(self._details)
        layout2 = Layout([1, 1])
        self.add_layout(layout2)
        layout2.add_widget(Button("Save", self._save), 0)
        layout2.add_widget(Button("Back", self._back), 1)
        self.fix()
        self.card_str = ""
        self.selected_file = None

    def reset(self):
        super(DetailFrame, self).reset()
        global LAST_SELECTED_FILE
        self.selected_file = LAST_SELECTED_FILE
        if self.selected_file:
            self._file_label.text = "File: " + self.selected_file
            card_ptr = c_void_p()
            ret = vc_parser.createCard(self.selected_file.encode("utf-8"), ctypes.byref(card_ptr))
            if ret == OK:
                card_str_ptr = vc_parser.cardToString(card_ptr)
                self.card_str = ctypes.string_at(card_str_ptr).decode("utf-8")
                self._contact.value = self.extractFN(self.card_str)
                self._details.value = self.card_str
                vc_parser.deleteCard(card_ptr)
            else:
                self._details.value = "Error loading card."
        else:
            self._file_label.text = "No file selected."
            self._details.value = ""

    def extractFN(self, card_str):
        for line in card_str.splitlines():
            if line.startswith("FN:"):
                return line[3:].strip()
        return ""

    def _save(self):
        new_fn = self._contact.value.strip()
        if new_fn == "":
            self.scene.add_effect(PopUpDialog(self.screen, "FN cannot be empty.", ["OK"]))
            return
        card_ptr = c_void_p()
        ret = vc_parser.createCard(self.selected_file.encode("utf-8"), ctypes.byref(card_ptr))
        if ret != OK:
            self.scene.add_effect(PopUpDialog(self.screen, "Error reloading card for update.", ["OK"]))
            return
        ret = vc_parser.updateFN(card_ptr, new_fn.encode("utf-8"))
        if ret != OK:
            self.scene.add_effect(PopUpDialog(self.screen, "Error updating FN.", ["OK"]))
            vc_parser.deleteCard(card_ptr)
            return
        ret = vc_parser.writeCard(self.selected_file.encode("utf-8"), card_ptr)
        if ret != OK:
            self.scene.add_effect(PopUpDialog(self.screen, "Error writing card to file.", ["OK"]))
        else:
            self.scene.add_effect(PopUpDialog(self.screen, "Card updated successfully.", ["OK"]))
            update_db_for_file(self.selected_file, new_fn, DB_CONN)
        vc_parser.deleteCard(card_ptr)
        raise NextScene("Main")

    def _back(self):
        raise NextScene("Main")

class CreateFrame(Frame):
    def __init__(self, screen):
        super(CreateFrame, self).__init__(screen,
                                          screen.height,
                                          screen.width,
                                          can_scroll=False,
                                          title="Create New vCard")
        layout = Layout([100])
        self.add_layout(layout)
        self._file = Text("File Name:", "filename")
        self._contact = Text("Contact:", "contact")
        layout.add_widget(self._file)
        layout.add_widget(self._contact)
        layout2 = Layout([1, 1])
        self.add_layout(layout2)
        layout2.add_widget(Button("Create", self._create), 0)
        layout2.add_widget(Button("Cancel", self._cancel), 1)
        self.fix()

    def _create(self):
        self.save()
        filename = self.data["filename"].strip()
        contact = self.data["contact"].strip()

        # Build absolute path to the 'cards' folder
        cards_dir = os.path.join(BASE_DIR, "cards")
        if not os.path.exists(cards_dir):
            os.makedirs(cards_dir)

        full_path = os.path.join(cards_dir, filename)
        if not full_path.lower().endswith((".vcf", ".vcard")):
            self.scene.add_effect(PopUpDialog(self.screen, "Invalid file extension.", ["OK"]))
            return

        if os.path.exists(full_path):
            self.scene.add_effect(PopUpDialog(self.screen, "File already exists.", ["OK"]))
            return

        card_ptr = vc_parser.createEmptyCard()
        if not card_ptr:
            self.scene.add_effect(PopUpDialog(self.screen, "Error creating empty card.", ["OK"]))
            return

        ret = vc_parser.updateFN(card_ptr, contact.encode("utf-8"))
        if ret != OK:
            self.scene.add_effect(PopUpDialog(self.screen, "Error updating FN.", ["OK"]))
            vc_parser.deleteCard(card_ptr)
            return

        ret = vc_parser.writeCard(full_path.encode("utf-8"), card_ptr)
        if ret != OK:
            self.scene.add_effect(PopUpDialog(self.screen, "Error writing card to file.", ["OK"]))
        else:
            self.scene.add_effect(PopUpDialog(self.screen, "Card created successfully.", ["OK"]))
            insert_db_for_new_file(full_path, contact, DB_CONN)
        vc_parser.deleteCard(card_ptr)
        raise NextScene("Main")

    def _cancel(self):
        raise NextScene("Main")

class DBFrame(Frame):
    def __init__(self, screen):
        super(DBFrame, self).__init__(screen,
                                      screen.height,
                                      screen.width,
                                      can_scroll=False,
                                      title="DB View")
        self.db_conn = DB_CONN
        layout = Layout([100])
        self.add_layout(layout)
        self._query_result = TextBox(10, "Query Result", readonly=True)
        layout.add_widget(self._query_result)
        layout2 = Layout([1, 1, 1])
        self.add_layout(layout2)
        layout2.add_widget(Button("Display All Contacts", self._query_all), 0)
        layout2.add_widget(Button("Find Contacts Born in June", self._query_june), 1)
        layout2.add_widget(Button("Back", self._back), 2)
        self.fix()

    def _query_all(self):
        try:
            cursor = self.db_conn.cursor()
            cursor.execute("SELECT CONTACT.name, CONTACT.birthday, FILE.file_name FROM CONTACT JOIN FILE ON CONTACT.file_id = FILE.file_id ORDER BY CONTACT.name")
            rows = cursor.fetchall()
            result = "All Contacts:\n"
            for row in rows:
                result += str(row) + "\n"
            self._query_result.value = result
            cursor.close()
        except Exception as e:
            self._query_result.value = "Error: " + str(e)

    def _query_june(self):
        try:
            cursor = self.db_conn.cursor()
            cursor.execute("SELECT name, birthday FROM CONTACT WHERE MONTH(birthday) = 6 ORDER BY birthday")
            rows = cursor.fetchall()
            result = "Contacts Born in June:\n"
            for row in rows:
                result += str(row) + "\n"
            self._query_result.value = result
            cursor.close()
        except Exception as e:
            self._query_result.value = "Error: " + str(e)

    def _back(self):
        raise NextScene("Main")

# ------------------------
# Main Application Logic
# ------------------------

def global_unhandled_input(event):
    if event in ("q", "Q"):
        raise StopApplication("User pressed quit")

def main(screen, scene):
    scenes = []

    # Login scene.
    login_frame = LoginFrame(screen)
    scenes.append(Scene([login_frame], -1, name="Login"))

    # Main scene.
    main_frame = MainFrame(screen)
    scenes.append(Scene([main_frame], -1, name="Main"))

    # Detail scene.
    detail_frame = DetailFrame(screen)
    scenes.append(Scene([detail_frame], -1, name="Detail"))

    # Create scene.
    create_frame = CreateFrame(screen)
    scenes.append(Scene([create_frame], -1, name="Create"))

    # DB scene.
    db_frame = DBFrame(screen)
    scenes.append(Scene([db_frame], -1, name="DB"))

    screen.play(scenes, stop_on_resize=True, start_scene=scene, unhandled_input=global_unhandled_input)

if __name__ == "__main__":
    last_scene = None
    while True:
        try:
            Screen.wrapper(main, catch_interrupt=True, arguments=[last_scene])
            sys.exit(0)
        except ResizeScreenError as e:
            last_scene = e.scene
