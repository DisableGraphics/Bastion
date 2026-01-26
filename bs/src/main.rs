use std::collections::{BTreeMap, HashSet};
use std::error::Error;
use std::fs::{File, Permissions};
use std::io::{self, BufRead, Write};
use std::os::unix::fs::PermissionsExt;
use std::usize;
use csv::Reader;
use ratatui::crossterm::event::{self, Event, KeyCode};
use ratatui::layout::{Constraint, Layout};
use ratatui::style::{Color, Modifier, Style};
use ratatui::text::{Line, Span};
use ratatui::widgets::{Block, Borders, List, ListItem, ListState, Paragraph};
use ratatui::{DefaultTerminal, Frame};

#[derive(Debug, Clone)]
struct Module {
    name: String,
    category: String,
    compile_cmd: String,
    src_path: String,
    out_path: String,
    selected: bool,
	is_module: bool // If it's a kernel module
}

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let mut modules = load_modules("modules.csv").expect("No modules.csv file found");

	if let Err(e) = sync_with_existing_list(&mut modules, "modules.lst") {
        eprintln!("Note: Could not sync previous selection: {}", e);
    }

    run_tui_selection(&mut modules)?;
    generate_compile_script(&modules)?;
	generate_compiled_list(&modules)?;
    generate_limine_conf(&modules)?;

    println!("Configuration saved. Run ./compile.sh to build.");
    Ok(())
}

fn sync_with_existing_list(mods: &mut [Module], path: &str) -> io::Result<()> {
    let file = match File::open(path) {
        Ok(f) => f,
        Err(ref e) if e.kind() == io::ErrorKind::NotFound => return Ok(()),
        Err(e) => return Err(e),
    };

    let reader = io::BufReader::new(file);
    let active_paths: HashSet<String> = reader.lines()
        .filter_map(Result::ok)
        .map(|s| s.trim().to_string())
        .collect();

    for m in mods {
        if active_paths.contains(&m.out_path) {
            m.selected = true;
        }
    }

    Ok(())
}

fn load_modules(path: &str) -> Result<Vec<Module>, Box<dyn std::error::Error>> {
    let mut rdr = Reader::from_path(path)?;
    let mut mods = Vec::new();
    for result in rdr.records() {
        let record = result?;
        mods.push(Module {
            name: record[0].to_string(),
            category: record[1].to_string(),
            compile_cmd: record[2].to_string(),
            src_path: record[3].to_string(),
            out_path: record[4].to_string(),
            selected: false,
			is_module: record[5].parse()?
        });
    }
    Ok(mods)
}

fn generate_compile_script(mods: &[Module]) -> std::io::Result<()> {
    let mut file = File::create("compile.sh")?;
    writeln!(file, "#!/bin/sh\nset -e")?;
	writeln!(file, "CURR=$PWD\n")?;
    writeln!(file, "# Kernel")?;
    writeln!(file, "./compile_kernel.sh")?;

    for m in mods.iter().filter(|m| m.selected) {
        writeln!(file, "\n# Mod: {}", m.name)?;
		writeln!(file, "cd {}", m.src_path)?;
        writeln!(file, "{}", m.compile_cmd)?;
		writeln!(file, "cd \"$CURR\"\n")?;
    }
	std::fs::set_permissions("compile.sh", Permissions::from_mode(0o777))?;
    Ok(())
}

fn generate_compiled_list(mods: &[Module]) -> std::io::Result<()> {
	let mut file = File::create("modules.lst")?;

    for m in mods.iter().filter(|m| m.selected) {
        writeln!(file, "{}", m.out_path)?;
    }
    Ok(())
}

fn generate_limine_conf(mods: &[Module]) -> std::io::Result<()> {
    let mut file = File::create("limine.conf")?;
    writeln!(file, "timeout=0\n\n:/Bastion")?;
    writeln!(file, "    protocol: limine")?;
    writeln!(file, "    path: boot():/boot/kernel.elf")?;

    for m in mods.iter().filter(|m| m.selected) {
		if m.is_module {
			let dest_path = &m.out_path[m.out_path.rfind('/').unwrap_or(usize::MAX)+1..];
			writeln!(file, "    module: boot():/boot/{}", dest_path)?;
		}
    }
    Ok(())
}

enum MenuEntry {
    Category(String),
    ModuleItem(usize),
}

pub struct App<'a> {
    modules: &'a mut Vec<Module>,
    flat_menu: Vec<MenuEntry>,
    state: ListState,
    exit: bool,
}

impl<'a> App<'a> {
    fn new(modules: &'a mut Vec<Module>) -> Self {
        let mut flat_menu = Vec::new();
        let mut grouped: BTreeMap<String, Vec<usize>> = BTreeMap::new();

        for (idx, m) in modules.iter().enumerate() {
            grouped.entry(m.category.clone()).or_default().push(idx);
        }

        for (cat, indices) in grouped {
            flat_menu.push(MenuEntry::Category(cat));
            for idx in indices {
                flat_menu.push(MenuEntry::ModuleItem(idx));
            }
        }

        let mut state = ListState::default();
        state.select(Some(0));

        Self {
            modules,
            flat_menu,
            state,
            exit: false,
        }
    }

    pub fn run(&mut self, terminal: &mut DefaultTerminal) -> io::Result<()> {
        while !self.exit {
            terminal.draw(|frame| self.draw(frame))?;
            self.handle_events()?;
        }
        Ok(())
    }

    fn draw(&mut self, frame: &mut Frame) {
        let area = frame.area();
        
        let chunks = Layout::default()
            .direction(ratatui::layout::Direction::Vertical)
            .constraints([Constraint::Min(0), Constraint::Length(3)])
            .split(area);

        let items: Vec<ListItem> = self.flat_menu.iter().map(|entry| {
            match entry {
                MenuEntry::Category(name) => {
                    ListItem::new(Line::from(vec![
                        Span::styled(format!(" ▼ {} ", name), Style::default().fg(Color::Yellow).add_modifier(Modifier::BOLD))
                    ]))
                }
                MenuEntry::ModuleItem(idx) => {
                    let m = &self.modules[*idx];
                    let check = if m.selected { "[X]" } else { "[ ]" };
                    let style = if m.selected { Style::default().fg(Color::Green) } else { Style::default() };
                    ListItem::new(Line::from(vec![
                        Span::raw("   "), // Indentation
                        Span::styled(format!("{} {} {}", check, m.name, match m.is_module { true => "(Kernel Module)", false => ""}), style)
                    ]))
                }
            }
        }).collect();

        let list = List::new(items)
            .block(Block::default().borders(Borders::ALL).title(" Bastion Module Configuration "))
            .highlight_style(Style::default().bg(Color::Indexed(237)).add_modifier(Modifier::BOLD))
            .highlight_symbol(">> ");

        frame.render_stateful_widget(list, chunks[0], &mut self.state);

        let help = Paragraph::new("Space/Enter: Toggle | Escape/Q: Save & Exit | Arrows: Navigate")
            .block(Block::default().borders(Borders::ALL));
        frame.render_widget(help, chunks[1]);
    }

    fn handle_events(&mut self) -> io::Result<()> {
        if let Event::Key(key) = event::read()? {
            if key.kind == event::KeyEventKind::Press {
                match key.code {
                    KeyCode::Char('q') | KeyCode::Esc => self.exit = true,
                    KeyCode::Up => self.prev(),
                    KeyCode::Down => self.next(),
                    KeyCode::Char(' ') | KeyCode::Enter => self.toggle(),
                    _ => {}
                }
            }
        }
        Ok(())
    }

    fn next(&mut self) {
        let i = match self.state.selected() {
            Some(i) => if i >= self.flat_menu.len() - 1 { 0 } else { i + 1 },
            None => 0,
        };
        self.state.select(Some(i));
    }

    fn prev(&mut self) {
        let i = match self.state.selected() {
            Some(i) => if i == 0 { self.flat_menu.len() - 1 } else { i - 1 },
            None => 0,
        };
        self.state.select(Some(i));
    }

    fn toggle(&mut self) {
        if let Some(i) = self.state.selected() {
            if let MenuEntry::ModuleItem(idx) = self.flat_menu[i] {
                self.modules[idx].selected = !self.modules[idx].selected;
            }
        }
    }
}

fn run_tui_selection(mods: &mut Vec<Module>) -> Result<(), Box<dyn Error>> {
    let mut terminal = ratatui::init();
    let result = App::new(mods).run(&mut terminal);
    ratatui::restore();
    result.map_err(|e| e.into())
}