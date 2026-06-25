/* © 2026 Juan Sebastian Alarcón Alarcón (@JamesArmstrong7). Prohibida su
copia, ingeniería inversa o estudio funcional. Ley 23 de 1982. */

// Viquom - Arphel MVP
// Escritorio de Retorno Contextual

class ViquomApp {
    constructor() {
        this.proyectos = [];
        this.nextId = 1;
        this.init();
    }

    init() {
        // Cargar proyectos desde localStorage
        this.cargarProyectos();
        // Renderizar la vista inicial
        this.renderizarProyectos();
        this.actualizarAccesosRapidos();
        // Configurar event listeners
        this.configurarEventListeners();
    }

    cargarProyectos() {
        const proyectosGuardados = localStorage.getItem('viquim-proyectos');
        if (proyectosGuardados) {
            try {
                const parsed = JSON.parse(proyectosGuardados);
                this.proyectos = parsed.proyectos || [];
                this.nextId = parsed.nextId || 1;
            } catch (e) {
                console.error('Error al cargar proyectos:', e);
                this.proyectos = [];
                this.nextId = 1;
            }
        }
    }

    guardarProyectos() {
        const data = {
            proyectos: this.proyectos,
            nextId: this.nextId
        };
        localStorage.setItem('viquim-proyectos', JSON.stringify(data));
    }

    crearProyecto(datos) {
        const proyecto = {
            id: this.nextId++,
            nombre: datos.nombre,
            tipo: datos.tipo,
            ruta: datos.ruta || '',
            ultima_actividad: new Date().toISOString().split('T')[0],
            contexto_retorno: {
                en_que_estaba: datos.en_que_estaba || '',
                porque_pauso: datos.porque_pauso || '',
                al_volver: datos.al_volver || '',
                proximo_paso: datos.proximo_paso || ''
            },
            estado: 'pausado', // Por defecto nuevo proyecto comienza como pausado para establecer contexto inicial
            accesos_rapidos: false
        };
        this.proyectos.push(proyecto);
        this.guardarProyectos();
        return proyecto;
    }

    actualizarProyecto(id, actualizaciones) {
        const indice = this.proyectos.findIndex(p => p.id === id);
        if (indice !== -1) {
            this.proyectos[indice] = { ...this.proyectos[indice], ...actualizaciones };
            this.guardarProyectos();
            return true;
        }
        return false;
    }

    eliminarProyecto(id) {
        this.proyectos = this.proyectos.filter(p => p.id !== id);
        this.guardarProyectos();
    }

    obtenerProyecto(id) {
        return this.proyectos.find(p => p.id === id);
    }

    renderizarProyectos() {
        const grid = document.getElementById('projects-grid');
        if (!grid) return;

        if (this.proyectos.length === 0) {
            grid.innerHTML = `
                <div class="empty-state">
                    <h3>¡Comienza tu primer proyecto!</h3>
                    <p>Agrega un proyecto para ver tu escritorio de retorno.</p>
                </div>
            `;
            return;
        }

        grid.innerHTML = this.proyectos.map(proyecto => this.crearTarjetaProyecto(proyecto)).join('');
    }

    crearTarjetaProyecto(proyecto) {
        return `
            <div class="project-card" data-id="${proyecto.id}">
                <div class="project-header">
                    <h3>${this.escapeHtml(proyecto.nombre)}</h3>
                    <div class="project-meta">
                        <span>${this.formatearFecha(proyecto.ultima_actividad)}</span>
                        <span class="project-status status-${this.obtenerClaseEstado(proyecto.estado)}">${this.capitalizarPrimeraLetra(proyecto.estado)}</span>
                    </div>
                </div>
                <div class="project-context">
                    <div class="context-label">Contexto de retorno</div>
                    <div class="context-text">
                        ${this.renderizarContextoRetorno(proyecto.contexto_retorno)}
                    </div>
                </div>
                <div class="project-actions">
                    <button class="btn-abrir" onclick="app.abrirProyecto(${proyecto.id})">Abrir Proyecto</button>
                    <button class="btn-actualizar" onclick="app.editarContexto(${proyecto.id})">Actualizar Contexto</button>
                </div>
            </div>
        `;
    }

    renderizarContextoRetorno(contexto) {
        const lineas = [];
        if (contexto.en_que_estaba.trim()) lineas.push(`<strong>Último trabajo:</strong> ${this.escapeHtml(contexto.en_que_estaba)}`);
        if (contexto.porque_pauso.trim()) lineas.push(`<strong>Pausado porque:</strong> ${this.escapeHtml(contexto.porque_pauso)}`);
        if (contexto.al_volver.trim()) lineas.push(`<strong>Al volver:</strong> ${this.escapeHtml(contexto.al_volver)}`);
        if (contexto.proximo_paso.trim()) lineas.push(`<strong>Próximo paso:</strong> ${this.escapeHtml(contexto.proximo_paso)}`);

        return lineas.map(linea => `<p>${linea}</p>`).join('') || '<p><em>Aún no hay contexto de retorno. Haz clic en "Actualizar Contexto" para agregarlo.</em></p>';
    }

    actualizarAccesosRapidos() {
        const contenedor = document.getElementById('quick-access');
        if (!contenedor) return;

        // Mostrar hasta 4 proyectos marcados como acceso rápido o los más recientes
        const proyectosParaMostrar = this.proyectos
            .filter(p => p.accesos_rapidos)
            .slice(0, 4)
            .concat(this.proyectos
                .filter(p => !p.accesos_rapidos)
                .sort((a, b) => new Date(b.ultima_actividad) - new Date(a.ultima_actividad))
                .slice(0, 4))
            .slice(0, 4);

        if (proyectosParaMostrar.length === 0) {
            contenedor.innerHTML = '<p><em>No hay accesos rápidos</em></p>';
            return;
        }

        contenedor.innerHTML = proyectosParaMostrar.map(proyecto =>
            `<div class="quick-access-item" data-id="${proyecto.id}" title="${this.escapeHtml(proyecto.nombre)}">
                ${this.truncarTexto(proyecto.nombre, 18)}
            </div>`).join('');
    }

    abrirProyecto(id) {
        const proyecto = this.obtenerProyecto(id);
        if (!proyecto) return;

        // Actualizar última actividad al abrir
        this.actualizarProyecto(id, { ultima_actividad: new Date().toISOString().split('T')[0] });
        this.renderizarProyectos();
        this.actualizarAccesosRapidos();

        // Intentar abrir la carpeta/archivo asociado
        if (proyecto.ruta) {
            try {
                // En una aplicación de escritorio real, usaríamos una API del sistema
                // Para este MVP web, simulamos con un alert o simplemente lo anotamos
                console.log(`Abriendo: ${proyecto.ruta}`);
                // En un entorno real podríamos usar:
                // require('electron').shell.openPath(proyecto.ruta);
                // O simplemente notificar al usuario
                alert(`Para abrir el proyecto, navega a:\n${proyecto.ruta}\n\n(En una versión de escritorio real, esto abriría el archivo/carpeta directamente)`);
            } catch (e) {
                console.error('Error al intentar abrir el proyecto:', e);
            }
        }
    }

    editarContexto(id) {
        const proyecto = this.obtenerProyecto(id);
        if (!proyecto) return;

        // Llenar el modal con los datos actuales
        document.getElementById('edit-project-id').value = id;
        document.getElementById('edit-en-que-estaba').value = proyecto.contexto_retorno.en_que_estaba || '';
        document.getElementById('edit-porque-pauso').value = proyecto.contexto_retorno.porque_pauso || '';
        document.getElementById('edit-al-volver').value = proyecto.contexto_retorno.al_volver || '';
        document.getElementById('edit-proximo-paso').value = proyecto.contexto_retorno.proximo_paso || '';

        // Mostrar el modal
        const modal = document.getElementById('edit-context-modal');
        modal.style.display = 'flex';
    }

    guardarContextoDesdeModal() {
        const id = parseInt(document.getElementById('edit-project-id').value);
        if (!id) return;

        const actualizaciones = {
            contexto_retorno: {
                en_que_estaba: document.getElementById('edit-en-que-estaba').value.trim(),
                porque_pauso: document.getElementById('edit-porque-pauso').value.trim(),
                al_volver: document.getElementById('edit-al-volver').value.trim(),
                proximo_paso: document.getElementById('edit-proximo-paso').value.trim()
            },
            ultima_actividad: new Date().toISOString().split('T')[0] // Actualizar fecha al guardar contexto
        };

        if (this.actualizarProyecto(id, actualizaciones)) {
            this.cerrarModal('edit-context-modal');
            this.renderizarProyectos();
            this.actualizarAccesosRapidos();
        }
    }

    abrirModalNuevoProyecto() {
        document.getElementById('new-project-modal').style.display = 'flex';
        // Limpiar formulario
        document.getElementById('new-project-form').reset();
        document.getElementById('project-name').focus();
    }

    crearProyectoDesdeModal() {
        const form = document.getElementById('new-project-form');
        if (!form.checkValidity()) {
            form.reportValidity();
            return;
        }

        const datos = {
            nombre: document.getElementById('project-name').value.trim(),
            tipo: document.getElementById('project-type').value,
            ruta: document.getElementById('project-path').value.trim(),
            en_que_estaba: '',
            porque_pauso: '',
            al_volver: '',
            proximo_paso: document.getElementById('initial-context').value.trim()
        };

        this.crearProyecto(datos);
        this.cerrarModal('new-project-modal');
        this.renderizarProyectos();
        this.actualizarAccesosRapidos();
    }

    cerrarModal(modalId) {
        const modal = document.getElementById(modalId);
        if (modal) {
            modal.style.display = 'none';
        }
    }

    configurarEventListeners() {
        // Botón Nuevo Proyecto en el header
        document.getElementById('btn-new-project').addEventListener('click', () => this.abrirModalNuevoProyecto());
        document.getElementById('btn-new-project-sidebar').addEventListener('click', () => this.abrirModalNuevoProyecto());

        // Envío del formulario de nuevo proyecto
        document.getElementById('new-project-form').addEventListener('submit', (e) => {
            e.preventDefault();
            this.crearProyectoDesdeModal();
        });

        // Envío del formulario de edición de contexto
        document.getElementById('edit-context-form').addEventListener('submit', (e) => {
            e.preventDefault();
            this.guardarContextoDesdeModal();
        });

        // Botones de cierre de modales
        document.getElementById('btn-close-modal').addEventListener('click', () => this.cerrarModal('new-project-modal'));
        document.getElementById('btn-cancel-project').addEventListener('click', () => this.cerrarModal('new-project-modal'));
        document.getElementById('btn-close-context-modal').addEventListener('click', () => this.cerrarModal('edit-context-modal'));
        document.getElementById('btn-cancel-context').addEventListener('click', () => this.cerrarModal('edit-context-modal'));

        // Cerrar modal al hacer click fuera del contenido
        window.addEventListener('click', (e) => {
            if (e.target.classList.contains('modal')) {
                e.target.style.display = 'none';
            }
        });

        // Click en elementos de acceso rápido
        document.getElementById('quick-access').addEventListener('click', (e) => {
            const item = e.target.closest('.quick-access-item');
            if (item) {
                const id = parseInt(item.dataset.id);
                if (!isNaN(id)) {
                    this.abrirProyecto(id);
                }
            }
        });

        // Tecla Escape para cerrar modales
        document.addEventListener('keydown', (e) => {
            if (e.key === 'Escape') {
                const modalesAbiertos = document.querySelectorAll('.modal[style*="display: flex"]');
                modalesAbiertos.forEach(modal => {
                    modal.style.display = 'none';
                });
            }
        });
    }

    // Métodos auxiliares
    escapeHtml(texto) {
        if (!texto) return '';
        return texto
            .replace(/&/g, '&amp;')
            .replace(/</g, '&lt;')
            .replace(/>/g, '&gt;')
            .replace(/"/g, '&quot;')
            .replace(/'/g, '&#039;');
    }

    formatearFecha(fechaString) {
        if (!fechaString) return 'Fecha desconocida';
        const fecha = new Date(fechaString);
        return fecha.toLocaleDateString('es-ES', {
            day: 'numeric',
            month: 'short',
            year: 'numeric'
        });
    }

    obtenerClaseEstado(estado) {
        switch (estado) {
            case 'activo': return 'activo';
            case 'pausado': return 'pausado';
            case 'completado': return 'completado';
            default: return 'pausado';
        }
    }

    capitalizarPrimeraLetra(palabra) {
        if (!palabra) return '';
        return palabra.charAt(0).toUpperCase() + palabra.slice(1);
    }

    truncarTexto(texto, longitudMaxima) {
        if (!texto || texto.length <= longitudMaxima) return texto;
        return texto.slice(0, longitudMaxima - 1) + '…';
    }
}

// Inicializar la aplicación cuando el DOM esté listo
document.addEventListener('DOMContentLoaded', () => {
    window.app = new ViquomApp();
});