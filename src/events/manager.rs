use hashbrown::HashMap;

use crate::context::Context;

use super::{Event, EventKind};

/// The minivector event manager
pub struct EventManager {
    /// The event queue
    queue: Vec<Event>,
    // The event handlers
    handlers: HashMap<EventKind, Box<fn(&mut Context, &Event)>>,
}

impl EventManager {
    /// Create a new event manager
    pub fn new() -> Self {
        Self {
            queue: Vec::new(),
            handlers: HashMap::new(),
        }
    }

    /// Register an event handler
    pub fn register(&mut self, kind: EventKind, handler: Box<fn(&mut Context, &Event)>) {
        self.handlers.insert(kind, handler);
    }

    /// Dispatch an event (this will add it to the queue)
    pub fn dispatch(&mut self, event: Event) {
        self.queue.push(event);
    }

    /// Clear the event queue
    pub fn clear(&mut self) {
        self.queue.clear();
    }

    /// Get the event queue
    pub fn queue(&self) -> Vec<Event> {
        self.queue.clone()
    }

    /// Get the event handlers
    pub fn handlers(&self) -> HashMap<EventKind, Box<fn(&mut Context, &Event)>> {
        self.handlers.clone()
    }
}
