#pragma once

#include <vector>

class PublicationAlreadyCreated : public std::exception {
	/*virtual */const char* what() const noexcept {
		return "PublicationAlreadyCreated";
	}
};

class PublicationNotFound : public std::exception {
	/*virtual */const char* what() const noexcept {
		return "PublicationNotFound";
	}
};

class TriedToRemoveRoot : public std::exception {
	/*virtual */const char* what() const noexcept {
		return "TriedToRemoveRoot";
	}
};

template <class Publication>
class CitationGraph {
private:
	using id_type = typename Publication::id_type;
	
public:
	// Tworzy nowy graf. Tworzy także węzeł publikacji o identyfikatorze stem_id.
	CitationGraph(const id_type &stem_id);

	// Konstruktor przenoszący i przenoszący operator przypisania. Powinny być
	// noexcept.
	CitationGraph(CitationGraph<Publication> &&other) noexcept;
	CitationGraph<Publication>& operator=(CitationGraph<Publication> &&other) noexcept;
	
	// Próba użycia konstruktora kopiującego lub kopiującego operatora przypisania
	// dla obiektów klasy CitationGraph powinna zakończyć się błędem kompilacji.
	CitationGraph(CitationGraph<Publication>&) = delete;
	CitationGraph<Publication>& operator=(CitationGraph<Publication>&) = delete;
	
	// Zwraca identyfikator źródła. Metoda ta powinna być noexcept wtedy i tylko
	// wtedy, gdy metoda Publication::get_id jest noexcept. Zamiast pytajnika należy
	// wpisać stosowne wyrażenie.
	id_type get_root_id() const noexcept(noexcept(Publication::get_id));

	// Zwraca listę identyfikatorów publikacji cytujących publikację o podanym
	// identyfikatorze. Zgłasza wyjątek PublicationNotFound, jeśli dana publikacja
	// nie istnieje.
	std::vector<id_type> get_children(const id_type &id) const;

	// Zwraca listę identyfikatorów publikacji cytowanych przez publikację o podanym
	// identyfikatorze. Zgłasza wyjątek PublicationNotFound, jeśli dana publikacja
	// nie istnieje.
	std::vector<id_type> get_parents(const id_type &id) const;

	// Sprawdza, czy publikacja o podanym identyfikatorze istnieje.
	bool exists(const id_type &id) const;

	// Zwraca referencję do obiektu reprezentującego publikację o podanym
	// identyfikatorze. Zgłasza wyjątek PublicationNotFound, jeśli żądana publikacja
	// nie istnieje.
	Publication& operator[](const id_type &id) const;

	// Tworzy węzeł reprezentujący nową publikację o identyfikatorze id cytującą
	// publikacje o podanym identyfikatorze parent_id lub podanych identyfikatorach
	// parent_ids. Zgłasza wyjątek PublicationAlreadyCreated, jeśli publikacja
	// o identyfikatorze id już istnieje. Zgłasza wyjątek PublicationNotFound, jeśli
	// któryś z wyspecyfikowanych poprzedników nie istnieje.
	void create(const id_type &id, const id_type &parent_id);
	void create(const id_type &id, const std::vector<id_type> &parent_ids);

	// Dodaje nową krawędź w grafie cytowań. Zgłasza wyjątek PublicationNotFound,
	// jeśli któraś z podanych publikacji nie istnieje.
	void add_citation(const id_type &child_id, const id_type &parent_id);

	// Usuwa publikację o podanym identyfikatorze. Zgłasza wyjątek
	// PublicationNotFound, jeśli żądana publikacja nie istnieje. Zgłasza wyjątek
	// TriedToRemoveRoot przy próbie usunięcia pierwotnej publikacji.
	void remove(const id_type &id);
};
